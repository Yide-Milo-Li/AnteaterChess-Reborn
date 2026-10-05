#include "app/application_controller.hpp"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>

using namespace ac;

int main(int argc, char **argv) {
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == "--version") {
        std::printf("AnteaterChess Reborn %s\n", AC_VERSION);
        return 0;
    }
    QQuickStyle::setStyle("Basic");
    if (argc > 1 && QString::fromLocal8Bit(argv[1]) == "--smoke-test")
        qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &message) {
            std::fprintf(stderr, "%s\n", message.toUtf8().constData());
        });
    QGuiApplication app(argc, argv);
    app.setOrganizationName("DeepAnteater");
    app.setApplicationName("AnteaterChess Reborn");
    ac::ApplicationController backend;
    if (!backend.valid())
        return 1;
    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/qt/qml");
    engine.setInitialProperties({
        {"controller", QVariant::fromValue(&backend)}
    });
    engine.load(QUrl("qrc:/qt/qml/AnteaterChess/Reborn/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(200, &app, [&] {
            bool resources = ac::verifyResources();
            bool retired = ac::retiredRuntimeLoaded();
            bool ok = resources && !retired;
            if (!ok)
                std::fprintf(stderr, "Resource check=%d, retired runtime loaded=%d\n", resources, retired);
            GameConfig config{};
            init_default_game_config(&config);
            ok = backend.start(config) == Status::Ok && ok;
            backend.finish();
            ok = backend.diagnostic() == Status::Ok && ok;
            backend.requestClose();
            app.exit(ok ? 0 : 1);
        });
    }
    return app.exec();
}
