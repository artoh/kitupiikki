#include "uusiyhteysdialog.h"

#include "ui_uusiyhteys.h"
#include "tilitietopalvelu.h"
#include "pankitmodel.h"
#include "db/tilimodel.h"

#include "db/kirjanpito.h"
#include "laskutus/iban.h"

#include <QSortFilterProxyModel>
#include <QMessageBox>

namespace Tilitieto {

UusiYhteysDialog::UusiYhteysDialog(TilitietoPalvelu *palvelu) :
    ui( new Ui::Uusiyhteys),
    palvelu_(palvelu)
{
    ui->setupUi(this);

    connect( ui->seuraavaNappi, &QPushButton::clicked, this, &UusiYhteysDialog::seuraava);
    connect( ui->suljeNappi, &QPushButton::clicked, this, &UusiYhteysDialog::hide);
    connect( ui->ValmisNappi, &QPushButton::clicked, this, &UusiYhteysDialog::valmis);
    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("asetukset/tilitiedot"); });
    connect( ui->pankkiView->selectionModel(), &QItemSelectionModel::selectionChanged,
             [this] {this->pankkiValittu(); } );
    connect( ui->pankkiView, &QListView::clicked, this, &UusiYhteysDialog::pankkiValittu);

    connect( palvelu_, &TilitietoPalvelu::vahvistaLinkilla, this, &UusiYhteysDialog::vahvista);

    QSortFilterProxyModel *filter = new QSortFilterProxyModel(this);
    filter->setSourceModel( palvelu->pankit() );
    filter->setSortRole(Qt::DisplayRole);
    filter->sort(0);
    ui->pankkiView->setModel(filter);

}

UusiYhteysDialog::~UusiYhteysDialog()
{
    delete ui;
}

void UusiYhteysDialog::lisaaValtuutus()
{
    show();

    ui->stackedWidget->setCurrentIndex(VALITSEPANKKI);    

    ui->seuraavaNappi->setVisible(true);
    ui->seuraavaNappi->setEnabled(false);
    ui->ValmisNappi->setVisible(false);

    // Yritetään esivalita pankki, johon on tili
    for(int i = 0; i < kp()->tilit()->rowCount(); i++) {
        Tili* tili = kp()->tilit()->tiliPIndeksilla(i);
        const QString ibanStr = tili->iban();
        if( !ibanStr.isEmpty()) {
            const QString bic = Iban(ibanStr).bic();
            if( !bic.isEmpty() && !palvelu_->onkoValtuutettu(bic)) {
                // Valitaan tämä aktiiviseksi
                for(int j = 0; j < ui->pankkiView->model()->rowCount(); j++) {
                    const QModelIndex index = ui->pankkiView->model()->index(j,0);
                    if( index.data(PankitModel::BicRooli).toString() == bic ) {
                        ui->pankkiView->setCurrentIndex(index);
                        pankkiValittu();
                        return;
                    }
                }
            }
        }
    }


    // Esivalitaan pankki
    for(int i=0; i < kp()->tilit()->rowCount(); i++) {
        Tili* tili = kp()->tilit()->tiliPIndeksilla(i);
        Iban iban( tili->iban());
        if( !iban.bic().isEmpty() && !palvelu_->onkoValtuutettu(iban.bic())) {
            ui->pankkiView->setCurrentIndex(
                        palvelu_->pankit()->index( palvelu_->pankit()->indeksiBicilla( iban.bic() ) ));
            pankkiValittu();
            break;
        }
    }
    ui->pankkiView->setCurrentIndex( ui->pankkiView->model()->index(0,0) ) ;
    pankkiValittu();

}

void UusiYhteysDialog::vahvista(const QString &linkki, int pankkiId)
{
    if( linkki.isEmpty()) {
        hide();
        QMessageBox::critical( qobject_cast<QWidget*>(parent()), tr("Pankkiyhteyden lisääminen epäonnistui"),
                                        tr("Pankkiyhteyden lisääminen epäonnistui. \n\nYritä myöhemmin uudelleen."));
        return;
    }

    asetaLogo(pankkiId);

    show();
    ui->stackedWidget->setCurrentIndex(LINKKI);
    ui->linkki->setText( QString("<a href=\"%1\">%1</a>").arg(linkki) );
    ui->seuraavaNappi->setVisible(false);
    ui->ValmisNappi->setVisible(true);
}

void UusiYhteysDialog::asetaLogo(int pankkiId)
{
    Pankki pankki = palvelu_->pankit()->pankki(pankkiId);
    if( pankki.id() ) {
        QImage kuva = pankki.logo();
        ui->logo->setPixmap(QPixmap::fromImage( kuva.scaled(64,64) ));
    } else {
        ui->logo->clear();
    }
}

void UusiYhteysDialog::seuraava()
{
    ui->seuraavaNappi->setEnabled(false);
    int pankki = ui->pankkiView->currentIndex().data(PankitModel::IdRooli).toInt();
    if( pankki ) {
        palvelu_->lisaaValtuutus(pankki);
    }
}

void UusiYhteysDialog::valmis()
{
    hide();
    palvelu_->lataa();
}

void UusiYhteysDialog::pankkiValittu()
{
    QModelIndex index = ui->pankkiView->currentIndex();
    const QString& bic = index.data(PankitModel::BicRooli).toString();
    ui->seuraavaNappi->setEnabled(index.isValid());
    ui->OPlabel->setVisible( bic == "OKOYFIHH");
}

} // namespace Tilitieto
