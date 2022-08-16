#include "bannermaaritys.h"

#include "ui_bannermaaritys.h"
#include "db/kirjanpito.h"
#include "model/bannermodel.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>>


BannerMaaritys::BannerMaaritys() :
    MaaritysWidget(nullptr),
    ui(new Ui::BannerMaaritys),
    model( kp()->bannerit() )
{
    ui->setupUi(this);
    ui->listView->setModel(model);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &BannerMaaritys::uusiBanner);

    ui->poistaNappi->setEnabled(false);

}

BannerMaaritys::~BannerMaaritys()
{

}

void BannerMaaritys::uusiBanner()
{
    QString tiedosto = QFileDialog::getOpenFileName(this,
                                                    tr("Valitse lisättävä bannerikuva"),
                                                    QString(),
                                                    tr("Kuvatiedostot (*.png *.jpg *.jpeg)"));
    if( tiedosto.isEmpty()) return;

    QImage kuva;
    kuva.load(tiedosto);

    if( kuva.isNull()) {
        QMessageBox::critical(this, tr("Bannerin lisääminen"), tr("Kuvan lukeminen tiedostosta %1 epäonnistui.").arg(tiedosto));
        return;
    }
    const QString nimi = QInputDialog::getText(this,
                                               tr("Bannerin lisääminen"),
                                               tr("Bannerin nimi \nNäytetään laskua luodessa banneria valittaessa, mutta ei tulosteta laskulle."));
    if( !nimi.isEmpty() ) {
        model->lisaa(nimi, kuva);
    }

}
