#include "bannermaaritys.h"

#include "ui_bannermaaritys.h"
#include "db/kirjanpito.h"
#include "model/bannermodel.h"
#include "bannerdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QSortFilterProxyModel>
#include <QRegularExpression>

BannerMaaritys::BannerMaaritys() :
    MaaritysWidget(nullptr),
    ui(new Ui::BannerMaaritys),
    model( kp()->bannerit() )
{        
    ui->setupUi(this);

    QSortFilterProxyModel* proxy = new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    proxy->setFilterRole(BannerModel::IdRooli);
    proxy->setFilterRegularExpression(QRegularExpression("\\w+"));
    proxy->setSortRole(BannerModel::NimiRooli);
    proxy->sort(0);

    ui->listView->setModel(proxy);

    connect( ui->uusiNappi, &QPushButton::clicked, this, &BannerMaaritys::uusiBanner);
    connect( ui->muokkaaNappi, &QPushButton::clicked, this, &BannerMaaritys::muokkaa);
    connect( ui->poistaNappi, &QPushButton::clicked, this, &BannerMaaritys::poista);

    connect( ui->listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &BannerMaaritys::paivitaNapit);

    paivitaNapit();

}

BannerMaaritys::~BannerMaaritys()
{

}

void BannerMaaritys::uusiBanner()
{
    BannerDialog dlg(this, model);
    dlg.exec();
}

void BannerMaaritys::muokkaa()
{
    if( ui->listView->currentIndex().isValid()) {
        BannerDialog dlg(this, model);
        dlg.muokkaa(ui->listView->currentIndex());
    }
}

void BannerMaaritys::poista()
{
    if( ui->listView->currentIndex().isValid()) {
        model->poista( ui->listView->currentIndex().data(BannerModel::IndeksiRooli).toInt() );
    }
}

void BannerMaaritys::paivitaNapit()
{
    bool aktiivinen = ui->listView->currentIndex().isValid();
    ui->muokkaaNappi->setEnabled(aktiivinen);
    ui->poistaNappi->setEnabled(aktiivinen);
}
