#include "app/session_adapter.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QtQuickTest>

using namespace ac;
class Setup : public QObject {
    Q_OBJECT
  public slots:
    void qmlEngineAvailable(QQmlEngine *engine) {
        adapter = new ac::SessionAdapter(nullptr, engine);
        engine->rootContext()->setContextProperty("backend", adapter);
    }

  private:
    ac::SessionAdapter *adapter = nullptr;
};
QUICK_TEST_MAIN_WITH_SETUP(anteater_qml, Setup)
#include "test_qml.moc"
