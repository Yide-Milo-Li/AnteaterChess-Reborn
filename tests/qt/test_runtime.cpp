#include "runtime/runtime.hpp"
#include "app/application_controller.hpp"
#include "../core/failing_resource.hpp"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

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
        ApplicationController controller(&options, nullptr, SessionLog(directory.path()), &resource);
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
        ApplicationController controller(&options);
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
        ApplicationController controller(&options, nullptr, std::move(log));
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
    void atomicFailurePreservesLastSnapshot() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        SessionLog log(directory.path());
        SessionSnapshot snapshot{};
        snapshot.gameId = 1;
        snapshot.elapsedMs = 42;
        QCOMPARE(log.write(snapshot), Status::Ok);
        QFile previous(log.path());
        QVERIFY(previous.open(QIODevice::ReadOnly));
        const auto bytes = previous.readAll();
        previous.close();
        snapshot.elapsedMs = 99;
#ifdef _WIN32
        // Deny replacement while allowing reads: QSaveFile can write its temp
        // file, but committing over the last accepted snapshot must fail.
        const auto path = log.path().toStdWString();
        HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        QVERIFY(handle != INVALID_HANDLE_VALUE);
        const auto status = log.write(snapshot);
        CloseHandle(handle);
#else
        const auto permissions = QFile::permissions(directory.path());
        QVERIFY(QFile::setPermissions(directory.path(), QFile::ReadOwner | QFile::ExeOwner));
        const auto status = log.write(snapshot);
        QVERIFY(QFile::setPermissions(directory.path(), permissions));
#endif
        QCOMPARE(status, Status::IoError);
        QVERIFY(previous.open(QIODevice::ReadOnly));
        QCOMPARE(previous.readAll(), bytes);
        previous.close();
        QCOMPARE(log.write(snapshot), Status::Ok);
        QVERIFY(previous.open(QIODevice::ReadOnly));
        QVERIFY(previous.readAll().contains("Elapsed ms: 99"));
    }
};
QTEST_GUILESS_MAIN(RuntimeTest)
#include "test_runtime.moc"
