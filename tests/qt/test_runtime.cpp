#include "runtime/runtime.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
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
        AcSnapshot snapshot{};
        snapshot.gameId = 1;
        QCOMPARE(a.write(snapshot),AC_OK);
        QCOMPARE(b.write(snapshot),AC_OK);
        QVERIFY(a.path() != b.path());
        QString first = a.path();
        snapshot.elapsedMs = 42;
        QCOMPARE(a.write(snapshot),AC_OK);
        QCOMPARE(a.path(),first);
        QFile file(first); QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("Elapsed ms: 42")); file.close();
        ++snapshot.gameId;
        QCOMPARE(a.write(snapshot),AC_OK);
        QVERIFY(a.path() != first);
        ac::SessionLog blocked(first);
        QCOMPARE(blocked.write(snapshot),AC_IO_ERROR);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("Elapsed ms: 42"));
    }
    void diagnosticOnlyFailure() {
        // A failed path resolver remains a real log object; moves stay accepted.
        ac::SessionLog log("");
        AcSessionOptions options{};
        options.clock = {ac::monotonicMilliseconds,nullptr};
        options.log = ac::SessionLog::writeCallback; options.logContext = &log;
        AcSession *s = ac_session_create(&options);
        QVERIFY(s);
        AcGameConfig c{}; ac_init_default_game_config(&c);
        QCOMPARE(ac_session_start(s,&c),AC_OK);
        AcMoveRequest request{};
        QCOMPARE(ac_parse_move_request_fields("E2","E4",AC_PROMOTION_CHOICE_NONE,&request),0);
        QCOMPARE(ac_session_submit(s,request),AC_OK);
        AcSnapshot snapshot{}; ac_session_snapshot(s,&snapshot);
        QCOMPARE(snapshot.historyCount,1);
        QCOMPARE(snapshot.diagnostic,AC_IO_ERROR);
        QVERIFY(log.path().isEmpty());
        ac_session_destroy(s);
    }
};
QTEST_GUILESS_MAIN(RuntimeTest)
#include "test_runtime.moc"
