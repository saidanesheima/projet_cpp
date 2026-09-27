#ifndef LIVRAISONSWIDGET_H
#define LIVRAISONSWIDGET_H

#include <QWidget>
#include "livraison.h"

namespace Ui {
class LivraisonsWidget;
}

class LivraisonsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LivraisonsWidget(QWidget *parent = nullptr);
    ~LivraisonsWidget();

private slots:
    void on_btnAjouter_clicked();
    void on_btnModifier_clicked();
    void on_btnSupprimer_clicked();
    void on_btnAfficher_clicked();

private:
    Ui::LivraisonsWidget *ui;
    Livraison tmpLivraison;
};

#endif // LIVRAISONSWIDGET_H