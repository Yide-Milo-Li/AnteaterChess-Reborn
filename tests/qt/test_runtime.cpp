#include "runtime/runtime.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace ac;
class RuntimeTest : public QObject {
    Q_OBJECT
  private slots:
    void pathsAndMonotonic() {
        QVERIFY(QDir::isAbsolutePath(ac::executableDirectory()));
        int64_t before = ac::monotonicMilliseconds(nullptr);
        QVERIFY(ac::monotonicMilliseconds(nullptr) >= before);
    }
    void logs() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ac::SessionLog a(directory.path()), b(directory.path());
        Snapshot snapshot{};
        snapshot.gameId = 1;
        QCOMPARE(a.write(snapshot), Status::Ok);
        QCOMPARE(b.write(snapshot), Status::Ok);
        QVERIFY(a.path() != b.path());
        QString first = a.path();
        snapshot.elapsedMs = 42;
        QCOMPARE(a.write(snapshot), Status::Ok);
        QCOMPARE(a.path(), first);
        QFile file(first);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("Elapsed ms: 42"));
        file.close();
        ++snapshot.gameId;
        QCOMPARE(a.write(snapshot), Status::Ok);
        QVERIFY(a.path() != first);
        ac::SessionLog blocked(first);
        QCOMPARE(blocked.write(snapshot), Status::IoError);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("Elapsed ms: 42"));
    }
    void diagnosticOnlyFailure() {
        // A failed path resolver remains a real log object; moves stay accepted.
        ac::SessionLog log("");
        SessionOptions options{};
        options.clock = {ac::monotonicMilliseconds, nullptr};
        options.log = ac::SessionLog::writeCallback;
        options.logContext = &log;
        Session *s = session_create(&options);
        QVERIFY(s);
        GameConfig c{};
        init_default_game_config(&c);
        QCOMPARE(session_start(s, &c), Status::Ok);
        MoveRequest request{};
        QCOMPARE(parse_move_request_fields("E2", "E4", PromotionChoice::None, &request), Status::Ok);
        QCOMPARE(session_submit(s, request), Status::Ok);
        Snapshot snapshot{};
        session_snapshot(s, &snapshot);
        QCOMPARE(snapshot.historyCount, 1);
        QCOMPARE(snapshot.diagnostic, Status::IoError);
        QVERIFY(log.path().isEmpty());
        session_destroy(s);
    }
};
QTEST_GUILESS_MAIN(RuntimeTest)
#include "test_runtime.moc"
