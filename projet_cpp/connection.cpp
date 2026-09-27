#include "connection.h"

Connection::Connection()
{
}

bool Connection::createconnect()
{
    bool test = false;
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");

    // BADEL HETHOMA B-LES DONNEES MTE3EK KAN LAZEM
    db.setDatabaseName("Source_Projet2A"); // Nom de la source de données ODBC
    db.setUserName("system");              // Nom d'utilisateur Oracle
    db.setPassword("esprit24");            // Mot de passe Oracle

    if (db.open())
        test = true;

    return test;
}