#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QLocale>
#include <QMessageBox>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Gestion des Ventes"));
    app.setOrganizationName(QStringLiteral("SalesDemo"));
    app.setStyle(QStringLiteral("Fusion"));
    app.setFont(QFont(QStringLiteral("Segoe UI"), 10));
    QLocale::setDefault(QLocale(QLocale::French, QLocale::France));

    QFile theme(QStringLiteral(":/styles/app.qss"));
    if (!theme.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(nullptr, QStringLiteral("Ressource manquante"),
                              QStringLiteral("Impossible de charger styles/app.qss."));
        return 1;
    }
    app.setStyleSheet(QString::fromUtf8(theme.readAll()));

    MainWindow window;
    window.show();
    return app.exec();
}
