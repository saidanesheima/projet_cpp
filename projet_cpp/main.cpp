#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include "connection.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Création mte3 l'instance de connexion
    Connection c;
    bool test = c.createconnect();

    MainWindow w;

    // Tester kan el connexion mchat mrigla
    if(test)
    {
        w.show();
        QMessageBox::information(nullptr, QObject::tr("Base de données"),
                                 QObject::tr("Connexion réussie avec succès !\n"
                                             "Cliquez sur Cancel pour quitter."), QMessageBox::Cancel);
    }
    else
    {
        w.show(); // N-khalliw l'interface tet'hall 7atta kan famma erreur bech nchoufouha
        QMessageBox::critical(nullptr, QObject::tr("Base de données"),
                              QObject::tr("Erreur de connexion.\n"
                                          "Veuillez vérifier vos paramètres OBDC/Oracle.\n"
                                          "Cliquez sur Cancel pour quitter."), QMessageBox::Cancel);
    }

    return a.exec();
}