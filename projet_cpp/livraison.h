#ifndef LIVRAISON_H
#define LIVRAISON_H

#include <QString>
#include <QDate>
#include <QSqlQuery>
#include <QSqlQueryModel>

class Livraison
{
private:
    int id;
    QString adresse;
    QDate dateLivraison;
    QString statut; // Ex: "En cours", "Livrée", "Annulée"
    double quantiteEau;

public:
    // Constructeurs
    Livraison();
    Livraison(int id, QString adresse, QDate dateLivraison, QString statut, double quantiteEau);

    // Getters
    int getId() const { return id; }
    QString getAdresse() const { return adresse; }
    QDate getDateLivraison() const { return dateLivraison; }
    QString getStatut() const { return statut; }
    double getQuantiteEau() const { return quantiteEau; }

    // Setters
    void setId(int id) { this->id = id; }
    void setAdresse(const QString &adresse) { this->adresse = adresse; }
    void setDateLivraison(const QDate &date) { this->dateLivraison = date; }
    void setStatut(const QString &statut) { this->statut = statut; }
    void setQuantiteEau(double quantite) { this->quantiteEau = quantite; }

    // Méthodes CRUD (Oracle DB)
    bool ajouter();
    bool supprimer(int id);
    bool modifier();
    QSqlQueryModel* afficher();
};

#endif // LIVRAISON_H