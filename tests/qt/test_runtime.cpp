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
    void searchAllocationFailures() {
        SessionSnapshot snapshot{};
        position_init(&snapshot.position);
        snapshot.gameId = 7;
        snapshot.revision = 11;
        bool succeeded = false;
        for (std::size_t failure = 1; failure < 64 && !succeeded; ++failure) {
            FailingResource resource;
            resource.failAt = failure;
            {
                SearchJobs jobs(nullptr, &resource);
                QSignalSpy completed(&jobs, &SearchJobs::completed);
                const auto started = jobs.start(snapshot, 13, true, 100, 1);
                if (started != Status::Ok) {
                    QCOMPARE(started, Status::OutOfMemory);
                    QVERIFY(!jobs.busy());
                    QCOMPARE(resource.live, std::size_t(0));
                } else {
                    QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 5000);
                    QVERIFY(!jobs.busy());
                    const auto outcome = jobs.outcome();
                    QCOMPARE(outcome.gameId, uint64_t(7));
                    QCOMPARE(outcome.revision, uint64_t(11));
                    QCOMPARE(outcome.generation, uint64_t(13));
                    if (outcome.result.status == Status::Ok)
                        succeeded = true;
                    else
                        QCOMPARE(outcome.result.status, Status::OutOfMemory);
                }
            }
            QCOMPARE(resource.live, std::size_t(0));
        }
        QVERIFY(succeeded);
    }
    void failedSearchIsNotRetried() {
        FailingResource resource;
        resource.failAt = 3; // Request and task are prepared; workspace creation fails.
        SessionOptions options{
            {ac::monotonicMilliseconds, nullptr}
        };
        QTemporaryDir directory;
        SessionAdapter controller(&options, nullptr, SessionLog(directory.path()), &resource);
        GameConfig config{};
        init_game_config_for_mode(&config, GameMode::ComputerVsComputer);
        QCOMPARE(controller.start(config), Status::Ok);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.state().historyCount, 0);
        QVERIFY(controller.statusError());
        const auto attempts = resource.attempts;
        for (int i = 0; i < 100; ++i)
            controller.tick();
        QCOMPARE(resource.attempts, attempts);
        resource.failAt = 0;
        QCOMPARE(controller.start(config), Status::Ok);
        QVERIFY(controller.busy());
        controller.requestClose();
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QCOMPARE(controller.state().historyCount, 0);
    }
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
