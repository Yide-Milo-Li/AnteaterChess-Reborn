#include "runtime/runtime.h"
#include "app/session_adapter.h"
#include "../core/failing_resource.hpp"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace ac;
class RuntimeTest : public QObject {
    Q_OBJECT
  private slots:
    void projectionAllocationFailure() {
        FailingResource resource;
        SessionOptions options{
            {ac::monotonicMilliseconds, nullptr},
            &resource
        };
        SessionAdapter controller(&options);
        QVERIFY(controller.valid());
        GameConfig config{};
        init_default_game_config(&config);
        resource.failNext();
        QCOMPARE(controller.start(config), Status::Ok);
        QCOMPARE(controller.state().phase, SessionPhase::Active);
        QCOMPARE(controller.diagnostic(), Status::OutOfMemory);
        resource.failAt = 0;
        QCOMPARE(controller.start(config), Status::Ok);
        QCOMPARE(controller.diagnostic(), Status::Ok);
    }
    void pathsAndMonotonic() {
        QVERIFY(QDir::isAbsolutePath(ac::executableDirectory()));
        int64_t before = ac::monotonicMilliseconds(nullptr);
        QVERIFY(ac::monotonicMilliseconds(nullptr) >= before);
    }
    void logs() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ac::SessionLog a(directory.path()), b(directory.path());
        SessionSnapshot snapshot{};
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
        SessionAdapter controller(&options, nullptr, std::move(log));
        QVERIFY(controller.valid());
        GameConfig c{};
        init_default_game_config(&c);
        QCOMPARE(controller.start(c), Status::Ok);
        controller.setMoveFields("E2", "E4");
        QVERIFY(controller.submitFields());
        QCOMPARE(controller.state().historyCount, 1);
        QCOMPARE(controller.diagnostic(), Status::IoError);
        QVERIFY(controller.statusError());
    }
};
QTEST_GUILESS_MAIN(RuntimeTest)
#include "test_runtime.moc"
