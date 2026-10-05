#include "app/application_controller.hpp"
#include "../core/failing_resource.hpp"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTemporaryDir>
#include <QtTest>

using namespace ac;
class DesktopTest : public QObject {
    Q_OBJECT
    struct Clock {
        int64_t ms = 0;
    };
    static int64_t now(void *p) {
        return static_cast<Clock *>(p)->ms;
    }
    static SessionOptions options(Clock &clock) {
        SessionOptions o{};
        o.clock = {now, &clock};
        return o;
    }
    static GameConfig config(GameMode mode = GameMode::HumanVsHuman) {
        GameConfig c{};
        init_game_config_for_mode(&c, mode);
        return c;
    }
  private slots:
    void ordinaryTicksNotifyOnlyClocks() {
        Clock clock;
        FailingResource resource;
        auto injected = options(clock);
        injected.resource = &resource;
        ApplicationController controller(&injected);
        auto c = config();
        c.timerEnabled = true;
        c.initialTimeSeconds = 10;
        QCOMPARE(controller.start(c), Status::Ok);
        controller.setMoveFields("E2", "E4");
        QSignalSpy board(controller.boardModel(), &QAbstractItemModel::dataChanged);
        QSignalSpy history(controller.historyModel(), &QAbstractItemModel::modelReset);
        QSignalSpy rows(controller.historyModel(), &QAbstractItemModel::rowsInserted);
        QSignalSpy fields(controller.input(), &InputModel::fieldsChanged);
        QSignalSpy validity(controller.input(), &InputModel::validityChanged);
        QSignalSpy availability(controller.game(), &StatusModel::availabilityChanged);
        QSignalSpy messages(controller.game(), &StatusModel::messageChanged);
        QSignalSpy turn(controller.game(), &StatusModel::turnChanged);
        QSignalSpy page(&controller, &ApplicationController::pageChanged);
        QSignalSpy elapsed(controller.clocks(), &ClockModel::elapsedChanged);
        const auto attempts = resource.attempts;
        const auto revision = controller.state().revision;
        for (int tick = 0; tick < 20; ++tick) {
            clock.ms += 100;
            controller.tick();
        }
        QCOMPARE(elapsed.count(), 2);
        QCOMPARE(board.count() + history.count() + rows.count() + fields.count() + validity.count() +
                     availability.count() + messages.count() + turn.count() + page.count(),
                 0);
        QCOMPARE(resource.attempts, attempts);
        QCOMPARE(controller.state().revision, revision);
        QCOMPARE(controller.fromText(), QString("E2"));
    }
    void settingsDraftValidation() {
        ApplicationController controller;
        controller.chooseMode(GameEnums::HumanVsComputer);
        controller.settings()->setPlayerColor(GameEnums::Black);
        controller.settings()->setWhiteDifficulty(GameEnums::Tournament);
        const auto before = controller.state();
        QCOMPARE(before.phase, SessionPhase::Idle);
        controller.settings()->setWhiteDifficulty(static_cast<GameEnums::Level>(4));
        QVERIFY(!controller.startDraft());
        QCOMPARE(controller.state().revision, before.revision);
        controller.settings()->setWhiteDifficulty(GameEnums::Easy);
        QVERIFY(controller.startDraft());
        QCOMPARE(controller.state().config.playerColor, Color::Black);
        QCOMPARE(controller.state().config.aiDifficultyWhite, Difficulty::Easy);
        QCOMPARE(controller.state().config.aiDifficultyBlack, Difficulty::None);
        controller.requestClose();
    }
    void historyScrollSurvivesTicks() {
        Clock clock;
        auto injected = options(clock);
        ApplicationController controller(&injected);
        QQmlApplicationEngine engine;
        engine.addImportPath("qrc:/qt/qml");
        engine.setInitialProperties({
            {"controller", QVariant::fromValue(&controller)}
        });
        engine.load(QUrl("qrc:/qt/qml/AnteaterChess/Reborn/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QCOMPARE(controller.start(config()), Status::Ok);
        const char *from[] = {"B1", "B8", "C3", "C6"};
        const char *to[] = {"C3", "C6", "B1", "B8"};
        for (int ply = 0; ply < 32; ++ply) {
            controller.setMoveFields(from[ply % 4], to[ply % 4]);
            QVERIFY(controller.submitFields());
        }
        auto *history = window->findChild<QQuickItem *>("historyView");
        QVERIFY(history);
        QTRY_VERIFY(history->property("contentHeight").toReal() > history->height() + 100);
        history->setProperty("followEnd", false);
        history->setProperty("contentY", 80.0);
        QTRY_COMPARE(history->property("contentY").toReal(), qreal(80.0));
        const auto scrolled = history->property("contentY");
        for (int tick = 0; tick < 20; ++tick) {
            clock.ms += 100;
            controller.tick();
        }
        QTest::qWait(150);
        QCOMPARE(history->property("contentY"), scrolled);
        QCOMPARE(controller.historyCount(), 32);
        window->close();
    }
    void initTestCase() {
        QQuickStyle::setStyle("Basic");
    }
    void resources() {
        QVERIFY(ac::verifyResources());
    }
    void fullscreenRestoration_data() {
        QTest::addColumn<bool>("maximized");
        QTest::addColumn<int>("exitKey");
        QTest::newRow("ordinary-escape") << false << int(Qt::Key_Escape);
        QTest::newRow("ordinary-f11") << false << int(Qt::Key_F11);
        QTest::newRow("maximized-escape") << true << int(Qt::Key_Escape);
        QTest::newRow("maximized-f11") << true << int(Qt::Key_F11);
    }
    void fullscreenRestoration() {
        QFETCH(bool, maximized);
        QFETCH(int, exitKey);
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        QQmlApplicationEngine engine;
        engine.addImportPath("qrc:/qt/qml");
        engine.setInitialProperties({
            {"controller", QVariant::fromValue(&a)}
        });
        engine.load(QUrl("qrc:/qt/qml/AnteaterChess/Reborn/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        auto expected = maximized ? QWindow::Maximized : QWindow::Windowed;
        if (maximized)
            window->showMaximized();
        QTRY_COMPARE(window->visibility(), expected);
        // Exercise both exit controls twice, so a later cycle cannot overwrite
        // the saved maximized state with fullscreen or a transition state.
        for (int cycle = 0; cycle < 2; ++cycle) {
            window->requestActivate();
            QVERIFY(QTest::qWaitForWindowActive(window));
            QTest::keyClick(window, Qt::Key_F11);
            QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
            QTRY_VERIFY(window->property("isFullscreen").toBool());
            window->requestActivate();
            QVERIFY(QTest::qWaitForWindowActive(window));
            QTest::keyClick(window, Qt::Key(exitKey));
            QTRY_COMPARE(window->visibility(), expected);
            QTRY_VERIFY(!window->property("isFullscreen").toBool());
        }
        window->close();
    }
    void commandsAndOwnedModels() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        QVERIFY(a.valid());
        QCOMPARE(a.start(config()), Status::Ok);
        a.setMoveFields(" e2 ", "e4");
        QVERIFY(a.fromValid());
        QVERIFY(a.toValid());
        QVERIFY(a.submitFields());
        QCOMPARE(a.historyCount(), 1);
        auto *history = qobject_cast<QAbstractItemModel *>(a.historyModel());
        QCOMPARE(history->rowCount(), 1);
        a.undo();
        QCOMPARE(a.historyCount(), 0);
        QCOMPARE(history->rowCount(), 0);
        a.selectSquare(6, 4, Qt::LeftButton);
        QCOMPARE(a.fromText(), QString("E2"));
        a.selectSquare(4, 4, Qt::RightButton);
        QCOMPARE(a.historyCount(), 1);
        auto before = a.state();
        a.setMoveFields("A9", "E9");
        QVERIFY(!a.submitFields());
        QCOMPARE(a.state().position.hash, before.position.hash);
    }
    void promotionCancellation() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        QCOMPARE(a.start(config()), Status::Ok);
        const char *moves[][2] = {
            {"E2", "E4"},
            {"D7", "D5"},
            {"E4", "D5"},
            {"A7", "A6"},
            {"D5", "D6"},
            {"A6", "A5"},
            {"D6", "C7"},
            {"A5", "A4"}
        };
        for (auto &move : moves) {
            a.setMoveFields(move[0], move[1]);
            QVERIFY(a.submitFields());
        }
        QSignalSpy requested(&a, &ac::ApplicationController::promotionRequested);
        a.setMoveFields("C7", "B8");
        QVERIFY(!a.submitFields());
        QCOMPARE(requested.count(), 1);
        uint64_t hash = a.state().position.hash;
        a.cancelPromotion();
        QCOMPARE(a.state().position.hash, hash);
        QVERIFY(!a.submitFields());
        QVERIFY(a.submitFields(GameEnums::PromoteKnight));
        auto promoted = a.state();
        QCOMPARE(get_piece(&promoted.position.board, {0, 1}).type, PieceType::Knight);
    }
    void hintInvalidatedByMoveAndUndo() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        QCOMPARE(a.start(config()), Status::Ok);
        QVERIFY(a.hint());
        QVERIFY(a.busy());
        a.setMoveFields("E2", "E4");
        QVERIFY(a.submitFields());
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(), 5000);
        QCOMPARE(a.historyCount(), 1);
        QVERIFY(a.hint());
        a.undo();
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(), 5000);
        QCOMPARE(a.historyCount(), 0);
        QVERIFY(!a.status().startsWith("Hint:"));
    }
    void timeoutRejectsQueuedResult() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        auto c = config(GameMode::HumanVsComputer);
        c.playerColor = Color::Black;
        c.aiDifficultyWhite = Difficulty::Easy;
        c.aiDifficultyBlack = Difficulty::None;
        c.timerEnabled = 1;
        c.initialTimeSeconds = 1;
        QCOMPARE(a.start(c), Status::Ok);
        QVERIFY(a.busy());
        // Complete/cancel ordering is deliberately held until the clock mutates.
        clock.ms = 2000;
        a.tick();
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(), 5000);
        QCOMPARE(a.historyCount(), 0);
        QCOMPARE(a.state().position.currentTurn, Color::Black);
    }
    void replacementNavigationAndClose() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        auto c = config(GameMode::ComputerVsComputer);
        c.aiDifficultyWhite = c.aiDifficultyBlack = Difficulty::Hard;
        QCOMPARE(a.start(c), Status::Ok);
        QVERIFY(a.busy());
        for (int i = 0; i < 5; ++i)
            QCOMPARE(a.start(config()), Status::Ok);
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(), 5000);
        QCOMPARE(a.historyCount(), 0);
        QCOMPARE(a.start(c), Status::Ok);
        a.back();
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(), 5000);
        QCOMPARE(a.page(), int(ac::ApplicationController::MainMenu));
        QCOMPARE(a.start(c), Status::Ok);
        QVERIFY(a.busy());
        QSignalSpy ready(&a, &ac::ApplicationController::closeReady);
        QVERIFY(!a.requestClose());
        QTRY_COMPARE_WITH_TIMEOUT(ready.count(), 1, 5000);
        QVERIFY(a.requestClose());
        QCOMPARE(a.historyCount(), 0);
    }
    void queuedCompletionAfterShutdown() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        QCOMPARE(a.start(config()), Status::Ok);
        ac::SearchJobs jobs;
        QSignalSpy completed(&jobs, &ac::SearchJobs::completed);
        QCOMPARE(jobs.start(std::get<SessionSnapshot>(a.snapshot()), 1, true, 100, 1), Status::Ok);
        auto *thread = jobs.findChild<QThread *>();
        QVERIFY(thread);
        // Join without pumping the GUI queue: finished has been posted, but its
        // main-thread completion has not executed when shutdown retires the job.
        QVERIFY(thread->wait(5000));
        QVERIFY(jobs.busy());
        jobs.shutdown();
        QVERIFY(!jobs.busy());
        QCOMPARE(jobs.start(std::get<SessionSnapshot>(a.snapshot()), 2, true, 100, 1), Status::Ok);
        QTRY_COMPARE_WITH_TIMEOUT(completed.count(), 1, 5000);
        QCOMPARE(jobs.outcome().generation, uint64_t(2));
        QVERIFY(!jobs.busy());
        QCoreApplication::sendPostedEvents(&jobs);
        QCOMPARE(completed.count(), 1);
    }
    void actualAiAndHintResults() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        auto c = config(GameMode::HumanVsComputer);
        QCOMPARE(a.start(c), Status::Ok);
        a.setMoveFields("E2", "E4");
        QVERIFY(a.submitFields());
        QTRY_COMPARE_WITH_TIMEOUT(a.historyCount(), 2, 5000);
        QVERIFY(a.humanTurn());
        QVERIFY(a.hint());
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(), 5000);
        QVERIFY(a.status().startsWith("Hint:"));
        QCOMPARE(a.historyCount(), 2);
        c = config(GameMode::ComputerVsComputer);
        QCOMPARE(a.start(c), Status::Ok);
        QTRY_VERIFY_WITH_TIMEOUT(a.historyCount() > 0, 5000);
        a.finish();
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(), 5000);
    }
    void renderedPagesAndFocus() {
        Clock clock;
        auto o = options(clock);
        ac::ApplicationController a(&o);
        QQmlApplicationEngine engine;
        engine.addImportPath("qrc:/qt/qml");
        engine.setInitialProperties({
            {"controller", QVariant::fromValue(&a)}
        });
        engine.load(QUrl("qrc:/qt/qml/AnteaterChess/Reborn/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QString captures = qEnvironmentVariable("AC_CAPTURE_DIR");
        if (!captures.isEmpty())
            QDir().mkpath(captures);
        auto capture = [&](const QString &name) {
            QTest::qWait(150);
            QImage image = window->grabWindow();
            QVERIFY(!image.isNull());
            qInfo() << "Capture" << name << "devicePixelRatio" << window->devicePixelRatio()
                    << "logicalSize" << window->size() << "imageSize" << image.size();
            if (qEnvironmentVariableIsSet("AC_EXPECT_DEVICE_PIXEL_RATIO")) {
                const auto expected = qEnvironmentVariable("AC_EXPECT_DEVICE_PIXEL_RATIO").toDouble();
                QVERIFY(qAbs(window->devicePixelRatio() - expected) < 0.01);
            }
            if (!captures.isEmpty())
                QVERIFY(image.save(QDir(captures).filePath(name + ".png")));
        };
        capture("menu");
        a.newGame();
        capture("modes");
        a.chooseMode(GameEnums::HumanVsHuman);
        capture("setup");
        QCOMPARE(a.start(config()), Status::Ok);
        capture("gameplay");
        auto *field = window->findChild<QObject *>("fromEntry");
        QVERIFY(field);
        auto *item = qobject_cast<QQuickItem *>(field);
        QVERIFY(item);
        item->forceActiveFocus();
        QTest::keyClick(window, Qt::Key_E);
        QTest::keyClick(window, Qt::Key_2);
        QTRY_COMPARE(a.fromText().toUpper(), QString("E2"));
        QString entered = field->property("text").toString();
        clock.ms += 1000;
        a.tick();
        QCOMPARE(field->property("text").toString(), entered);
        QVERIFY(item->hasActiveFocus());
        window->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(window));
        QTest::keyClick(window, Qt::Key_F11);
        QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
        capture("fullscreen");
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_COMPARE(window->visibility(), QWindow::Windowed);
        const char *moves[][2] = {
            {"E2", "E4"},
            {"D7", "D5"},
            {"E4", "D5"},
            {"A7", "A6"},
            {"D5", "D6"},
            {"A6", "A5"},
            {"D6", "C7"},
            {"A5", "A4"}
        };
        for (auto &move : moves) {
            a.setMoveFields(move[0], move[1]);
            QVERIFY(a.submitFields());
        }
        a.setMoveFields("C7", "B8");
        QVERIFY(!a.submitFields());
        auto *promotion = window->findChild<QObject *>("promotionDialog");
        QVERIFY(promotion);
        QTRY_VERIFY(promotion->property("visible").toBool());
        capture("promotion");
        uint64_t before = a.state().position.hash;
        auto *cancel = window->findChild<QQuickItem *>("promotionCancel");
        QVERIFY(cancel);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                          cancel->mapToScene(QPointF(cancel->width() / 2, cancel->height() / 2)).toPoint());
        QTRY_VERIFY(!promotion->property("visible").toBool());
        QCOMPARE(a.state().position.hash, before);
        auto *confirmation = window->findChild<QObject *>("confirmationDialog");
        QVERIFY(confirmation);
        confirmation->setProperty("action", "finish");
        QVERIFY(QMetaObject::invokeMethod(confirmation, "open"));
        capture("confirmation");
        QVERIFY(QMetaObject::invokeMethod(confirmation, "reject"));
        QCOMPARE(a.page(), int(ac::ApplicationController::Gameplay));
        QVERIFY(QMetaObject::invokeMethod(confirmation, "open"));
        QVERIFY(QMetaObject::invokeMethod(confirmation, "accept"));
        QTRY_COMPARE(a.page(), int(ac::ApplicationController::EndGame));
        capture("endgame");
        window->close();
    }
};
QTEST_MAIN(DesktopTest)
#include "test_desktop.moc"
