#include "eranselvityseranvalintadialog.h"
#include "ui_eranvalintadialog.h"


EranSelvitysEranValintaDialog::EranSelvitysEranValintaDialog(EranSelvitysEraModel *model, QWidget *parent)
    : EranValintaDialog(parent)
{
    asetaModel(model);
    ui->avoimetCheck->hide();
    ui->view->horizontalHeader()->setSectionResizeMode(EranSelvitysEraModel::SELITE, QHeaderView::Stretch);
}

int EranSelvitysEranValintaDialog::valitseEra(int nykyinen)
{
    asetaNykyinen(nykyinen);
    if( exec() == QDialog::Accepted )
        return valittu().value("id").toInt();
    return nykyinen;
}
