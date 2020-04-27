#include <QApplication>
#include <QWidget>

#include "qwinwidget.h"

int main(int argc, char *argv[]) {
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication application(argc, argv);

    // The content widget must be a pointer.
    QWidget *widget = new QWidget();
    // The QWinWidget must not be a pointer.
    QWinWidget winWidget(widget);
    winWidget.show();

    const int exec = QApplication::exec();

    delete widget;

    return exec;
}
