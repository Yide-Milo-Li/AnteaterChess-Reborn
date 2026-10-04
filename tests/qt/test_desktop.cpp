#include "app/session_adapter.h"
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTemporaryDir>
#include <QtTest>
class DesktopTest : public QObject {
    Q_OBJECT
    struct Clock { int64_t ms = 0; };
    static int64_t now(void *p) { return static_cast<Clock *>(p)->ms; }
    static AcSessionOptions options(Clock &clock) {
        AcSessionOptions o{}; o.clock = {now,&clock}; return o;
    }
    static AcGameConfig config(AcGameMode mode = AC_MODE_HUMAN_VS_HUMAN) {
        AcGameConfig c{}; ac_init_game_config_for_mode(&c,mode); return c;
    }
private slots:
    void resources() { QVERIFY(ac::verifyResources()); }
    void commandsAndOwnedModels() {
        Clock clock; auto o = options(clock); ac::SessionAdapter a(&o);
        QVERIFY(a.valid()); QCOMPARE(a.start(config()),AC_OK);
        a.setMoveFields(" e2 ","e4"); QVERIFY(a.fromValid()); QVERIFY(a.toValid());
        QVERIFY(a.submitFields()); QCOMPARE(a.historyCount(),1);
        auto *history = qobject_cast<QAbstractItemModel *>(a.historyModel());
        QCOMPARE(history->rowCount(),1);
        a.undo(); QCOMPARE(a.historyCount(),0); QCOMPARE(history->rowCount(),0);
        a.selectSquare(6,4,1); QCOMPARE(a.fromText(),QString("E2"));
        a.selectSquare(4,4,2); QCOMPARE(a.historyCount(),1);
        auto before = a.snapshot(); a.setMoveFields("A9","E9"); QVERIFY(!a.submitFields());
        QCOMPARE(a.snapshot().position.hash,before.position.hash);
    }
    void promotionCancellation() {
        Clock clock; auto o = options(clock); ac::SessionAdapter a(&o);
        QCOMPARE(a.start(config()),AC_OK);
        const char *moves[][2] = {{"E2","E4"},{"D7","D5"},{"E4","D5"},{"A7","A6"},
            {"D5","D6"},{"A6","A5"},{"D6","C7"},{"A5","A4"}};
        for (auto &move : moves) { a.setMoveFields(move[0],move[1]); QVERIFY(a.submitFields()); }
        QSignalSpy requested(&a,&ac::SessionAdapter::promotionRequested);
        a.setMoveFields("C7","B8"); QVERIFY(!a.submitFields()); QCOMPARE(requested.count(),1);
        uint64_t hash = a.snapshot().position.hash;
        a.cancelPromotion(); QCOMPARE(a.snapshot().position.hash,hash);
        QVERIFY(!a.submitFields()); QVERIFY(a.submitFields(AC_PROMOTION_CHOICE_KNIGHT));
        auto promoted = a.snapshot();
        QCOMPARE(ac_get_piece(&promoted.position.board,{0,1}).type,AC_KNIGHT);
    }
    void hintInvalidatedByMoveAndUndo() {
        Clock clock; auto o = options(clock); ac::SessionAdapter a(&o);
        QCOMPARE(a.start(config()),AC_OK); QVERIFY(a.hint()); QVERIFY(a.busy());
        a.setMoveFields("E2","E4"); QVERIFY(a.submitFields());
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(),5000);
        QCOMPARE(a.historyCount(),1);
        QVERIFY(a.hint()); a.undo();
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(),5000); QCOMPARE(a.historyCount(),0);
        QVERIFY(!a.status().startsWith("Hint:"));
    }
    void timeoutRejectsQueuedResult() {
        Clock clock; auto o = options(clock); ac::SessionAdapter a(&o);
        auto c = config(AC_MODE_HUMAN_VS_COMPUTER); c.playerColor = AC_BLACK;
        c.aiDifficultyWhite = AC_DIFFICULTY_EASY; c.aiDifficultyBlack = AC_DIFFICULTY_NONE;
        c.timerEnabled = 1; c.initialTimeSeconds = 1;
        QCOMPARE(a.start(c),AC_OK); QVERIFY(a.busy());
        // Complete/cancel ordering is deliberately held until the clock mutates.
        clock.ms = 2000; a.tick();
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(),5000);
        QCOMPARE(a.historyCount(),0); QCOMPARE(a.snapshot().position.currentTurn,AC_BLACK);
    }
    void replacementNavigationAndClose() {
        Clock clock; auto o = options(clock); ac::SessionAdapter a(&o);
        auto c = config(AC_MODE_COMPUTER_VS_COMPUTER);
        c.aiDifficultyWhite = c.aiDifficultyBlack = AC_DIFFICULTY_HARD;
        QCOMPARE(a.start(c),AC_OK); QVERIFY(a.busy());
        for (int i=0;i<5;++i) QCOMPARE(a.start(config()),AC_OK);
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(),5000); QCOMPARE(a.historyCount(),0);
        QCOMPARE(a.start(c),AC_OK); a.back();
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(),5000); QCOMPARE(a.page(),int(ac::SessionAdapter::MainMenu));
        QCOMPARE(a.start(c),AC_OK); QVERIFY(a.busy());
        QSignalSpy ready(&a,&ac::SessionAdapter::closeReady);
        QVERIFY(!a.requestClose()); QTRY_COMPARE_WITH_TIMEOUT(ready.count(),1,5000);
        QVERIFY(a.requestClose()); QCOMPARE(a.historyCount(),0);
    }
    void actualAiAndHintResults() {
        Clock clock; auto o = options(clock); ac::SessionAdapter a(&o);
        auto c = config(AC_MODE_HUMAN_VS_COMPUTER);
        QCOMPARE(a.start(c),AC_OK);
        a.setMoveFields("E2","E4"); QVERIFY(a.submitFields());
        QTRY_COMPARE_WITH_TIMEOUT(a.historyCount(),2,5000);
        QVERIFY(a.humanTurn()); QVERIFY(a.hint());
        QTRY_VERIFY_WITH_TIMEOUT(!a.busy(),5000);
        QVERIFY(a.status().startsWith("Hint:")); QCOMPARE(a.historyCount(),2);
        c = config(AC_MODE_COMPUTER_VS_COMPUTER);
        QCOMPARE(a.start(c),AC_OK);
        QTRY_VERIFY_WITH_TIMEOUT(a.historyCount()>0,5000);
        a.finish(); QTRY_VERIFY_WITH_TIMEOUT(!a.busy(),5000);
    }
    void renderedPagesAndFocus() {
        Clock clock; auto o = options(clock); ac::SessionAdapter a(&o);
        QQuickStyle::setStyle("Basic");
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("backend",&a);
        engine.load(QUrl("qrc:/qml/Main.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window); QVERIFY(QTest::qWaitForWindowExposed(window));
        QString captures = qEnvironmentVariable("AC_CAPTURE_DIR");
        if (!captures.isEmpty()) QDir().mkpath(captures);
        auto capture = [&](const QString &name) {
            QTest::qWait(150);
            QImage image = window->grabWindow(); QVERIFY(!image.isNull());
            if (!captures.isEmpty()) QVERIFY(image.save(QDir(captures).filePath(name+".png")));
        };
        capture("menu"); a.newGame(); capture("modes"); a.chooseMode(0); capture("setup");
        QCOMPARE(a.start(config()),AC_OK); capture("gameplay");
        auto *field = window->findChild<QObject *>("fromEntry"); QVERIFY(field);
        field->setProperty("text","E2");
        a.setMoveFields("E2","");
        auto *item = qobject_cast<QQuickItem *>(field); QVERIFY(item); item->forceActiveFocus();
        clock.ms += 1000; a.tick();
        QCOMPARE(field->property("text").toString(),QString("E2")); QVERIFY(item->hasActiveFocus());
        QTest::keyClick(window,Qt::Key_F11);
        QTRY_COMPARE(window->visibility(),QWindow::FullScreen); capture("fullscreen");
        QTest::keyClick(window,Qt::Key_Escape);
        QTRY_COMPARE(window->visibility(),QWindow::Windowed);
        a.finish(); capture("endgame"); window->close();
    }
};
QTEST_MAIN(DesktopTest)
#include "test_desktop.moc"
