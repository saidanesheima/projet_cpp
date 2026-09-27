#include "livraison.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

// Constructeur par défaut
Livraison::Livraison()
{
    id = 0;
    adresse = "";
    statut = "";
    quantiteEau = 0.0;
}

// Constructeur paramétré
Livraison::Livraison(int id, QString adresse, QDate dateLivraison, QString statut, double quantiteEau)
{
    this->id = id;
    this->adresse = adresse;
    this->dateLivraison = dateLivraison;
    this->statut = statut;
    this->quantiteEau = quantiteEau;
}

// Méthode Ajouter
bool Livraison::ajouter()
{
    QSqlQuery query;
    // PREPARE: n-7adhrou el requête SQL
    query.prepare("INSERT INTO LIVRAISONS (ID, ADRESSE, DATE_LIVRAISON, STATUT, QUANTITE_EAU) "
                  "VALUES (:id, :adresse, :date, :statut, :quantite)");

    // BIND: n-rebtou les variables bel valeurs
    query.bindValue(":id", id);
    query.bindValue(":adresse", adresse);
    query.bindValue(":date", dateLivraison);
    query.bindValue(":statut", statut);
    query.bindValue(":quantite", quantiteEau);

    return query.exec(); // T-raja3 true kan t-zadet mrigla
}

// Méthode Afficher
QSqlQueryModel* Livraison::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    model->setQuery("SELECT * FROM LIVRAISONS");

    // N-badlou asami les colonnes fil tableau (Optionnel)
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Adresse"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Statut"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Quantité (L)"));

    return model;
}

// Méthode Supprimer
bool Livraison::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM LIVRAISONS WHERE ID = :id");
    query.bindValue(":id", id);

    return query.exec();
}

// Méthode Modifier
bool Livraison::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE LIVRAISONS SET ADRESSE = :adresse, DATE_LIVRAISON = :date, "
                  "STATUT = :statut, QUANTITE_EAU = :quantite WHERE ID = :id");

    query.bindValue(":id", id);
    query.bindValue(":adresse", adresse);
    query.bindValue(":date", dateLivraison);
    query.bindValue(":statut", statut);
    query.bindValue(":quantite", quantiteEau);

    return query.exec();
}