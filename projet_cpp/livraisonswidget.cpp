#include "livraisonswidget.h"
#include "ui_livraisonswidget.h"

LivraisonsWidget::LivraisonsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LivraisonsWidget)
{
    ui->setupUi(this);
}

LivraisonsWidget::~LivraisonsWidget()
{
    delete ui;
}
