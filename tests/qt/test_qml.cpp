#include "app/application_controller.hpp"
#include <QQmlEngine>
#include <QQmlContext>
#include <QtQuickTest>

using namespace ac;
class Setup : public QObject {
    Q_OBJECT
  public slots:
    void qmlEngineAvailable(QQmlEngine *engine) {
        engine->addImportPath("qrc:/qt/qml");
    }

};
QUICK_TEST_MAIN_WITH_SETUP(anteater_qml, Setup)
#include "test_qml.moc"
