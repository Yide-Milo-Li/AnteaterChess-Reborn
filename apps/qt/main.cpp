#include "app/session_adapter.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>

int main(int argc, char **argv) {
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--version") {
        std::printf("AnteaterChess Reborn %s\n",AC_VERSION);
        return 0;
    }
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc,argv);
    app.setOrganizationName("DeepAnteater");
    app.setApplicationName("AnteaterChess Reborn");
    ac::SessionAdapter backend;
    if (!backend.valid()) return 1;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend",&backend);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(200,&app,[&] {
            bool ok = ac::verifyResources();
            AcGameConfig config{};
            ac_init_default_game_config(&config);
            ok = backend.start(config) == AC_OK && ok;
            backend.finish();
            ok = backend.snapshot().diagnostic == AC_OK && ok;
            backend.requestClose();
            app.exit(ok ? 0 : 1);
        });
    }
    return app.exec();
}
