#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>

#include "applicationmodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<ApplicationModel>("TweakGeek", 1, 0, "ApplicationModel");
    qmlRegisterUncreatableType<ApplicationItem>("TweakGeek", 1, 0, "ApplicationItem", "bla");

    QQuickView view;
    view.setSource(QUrl(QStringLiteral("qrc:///Main.qml")));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();
    return app.exec();
}

