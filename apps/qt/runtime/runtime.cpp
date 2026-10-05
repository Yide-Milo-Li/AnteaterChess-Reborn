#include "runtime.h"
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <QSaveFile>
#include <QTextStream>
#include <QUuid>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>

using namespace ac;
#endif

namespace ac {
bool retiredRuntimeLoaded() {
#ifdef _WIN32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE)
        return true;
    MODULEENTRY32W module{};
    module.dwSize = sizeof(module);
    bool retired = false;
    if (!Module32FirstW(snapshot, &module))
        retired = true;
    else
        do {
            QString name = QString::fromWCharArray(module.szModule).toLower();
            for (const char *prefix : {"libgtk", "libgdk", "libcairo", "libpango", "libatk", "libpixbuf"})
                if (name.startsWith(prefix))
                    retired = true;
        } while (Module32NextW(snapshot, &module));
    CloseHandle(snapshot);
    return retired;
#else
    return false;
#endif
}
QString executableDirectory() {
#ifdef _WIN32
    wchar_t buffer[32768];
    DWORD length = GetModuleFileNameW(nullptr, buffer, 32768);
    if (!length || length >= 32768)
        return {};
    QString executable = QString::fromWCharArray(buffer, int(length));
#else
    QString executable = QFileInfo(QStringLiteral("/proc/self/exe")).symLinkTarget();
#endif
    if (executable.isEmpty() || !QDir::isAbsolutePath(executable))
        return {};
    return QFileInfo(executable).absolutePath();
}
int64_t monotonicMilliseconds(void *unused) {
    Q_UNUSED(unused)
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
QString pieceAsset(Piece piece) {
    static const char *names[] = {"Ant", "Rook", "Knight", "Bishop", "Queen", "King", "Anteater"};
    if (piece.type < PieceType::Ant || piece.type > PieceType::Anteater || piece.color > Color::Black)
        return {};
    QString name = QString(piece.color == Color::White ? "White" : "Black") + names[enum_index(piece.type)];
    if (piece.type == PieceType::Ant && piece.color == Color::White)
        name = "WhiteAntsvg";
    return "qrc:/org/anteater/reborn/" + name + ".svg";
}
bool verifyResources() {
    auto render = [](const QString &path, int size) {
        // Exercise both the image plugin used by QML and vector rendering at
        // the requested size, instead of only scaling a default raster.
        QImage source(path);
        QSvgRenderer svg(path);
        if (source.isNull() || !svg.isValid())
            return false;
        QImage target(size, size, QImage::Format_ARGB32_Premultiplied);
        target.fill(Qt::transparent);
        QPainter painter(&target);
        svg.render(&painter);
        return !target.isNull();
    };
    const char *icons[] = {"icon-alert-dark.svg", "icon-hint-dark.svg", "icon-history-dark.svg", "icon-info-dark.svg"};
    for (int size : {32, 96, 160}) {
        for (int color = value(Color::White); color <= value(Color::Black); ++color)
            for (int type = value(PieceType::Ant); type <= value(PieceType::Anteater); ++type) {
                QString path = pieceAsset(create_piece(PieceType(type), Color(color))).mid(3);
                if (!render(path, size))
                    return false;
            }
        for (const char *name : icons) {
            if (!render(QString(":/org/anteater/reborn/") + name, size))
                return false;
        }
    }
    return true;
}
SessionLog::SessionLog(const QString &directory) : directory_(directory) {
}
SessionLog SessionLog::besideExecutable() {
    QString base = executableDirectory();
    return SessionLog(base.isEmpty() ? QString() : QDir(base).filePath("logs"));
}
Status SessionLog::writeCallback(void *context, const Snapshot *s) {
    return context && s ? static_cast<SessionLog *>(context)->write(*s) : Status::IoError;
}
Status SessionLog::write(const Snapshot &s) {
    if (directory_.isEmpty())
        return Status::IoError;
    if (path_.isEmpty() || gameId_ != s.gameId) {
        path_ = QDir(directory_).filePath("session-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".log");
        gameId_ = s.gameId;
    }
    if (!QDir().mkpath(directory_))
        return Status::IoError;
    QByteArray bytes;
    QTextStream text(&bytes, QIODevice::WriteOnly);
    text << "AnteaterChess Reborn " << AC_VERSION << '\n';
    text << "Mode: " << value(s.config.mode) << "\nTurn timer: " << s.config.initialTimeSeconds << " seconds ("
         << (s.config.timerEnabled ? "enabled" : "disabled") << ")\n";
    for (int i = 0; i < s.historyCount; ++i) {
        const Move &m = s.history[i];
        text << i + 1 << ". " << (m.movedPiece.color == Color::White ? "White" : "Black") << ' '
             << QChar('A' + m.from.col) << 8 - m.from.row << " -> " << QChar('A' + m.to.col) << 8 - m.to.row
             << " special=" << value(m.specialType) << " captures=" << m.captureCount;
        for (int k = 0; k < m.captureCount; ++k)
            text << ' ' << QChar('A' + m.captures[k].pos.col) << 8 - m.captures[k].pos.row;
        text << '\n';
    }
    text << "Elapsed ms: " << s.elapsedMs << "\nResult: " << value(s.result) << '\n';
    text.flush();
    // Atomic replacement preserves the previous snapshot on I/O failure.
    QSaveFile file(path_);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        return Status::IoError;
    return Status::Ok;
}
} // namespace ac
