#include "uusiyhteysdialog.h"

#include "ui_uusiyhteys.h"
#include "tilitietopalvelu.h"
#include "pankitmodel.h"
#include "db/tilimodel.h"

#include "db/kirjanpito.h"
#include "laskutus/iban.h"

#include "db/asetusmodel.h"

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

    if( kp()->asetukset()->onko(AsetusModel::TilitietoMaksuHyvaksytty)) {
        ui->stackedWidget->setCurrentIndex(VALITSEPANKKI);
    } else {
        ui->stackedWidget->setCurrentIndex(MAKSUINFO);
        ui->infoLabel->setText(tr("Tilitietojen noutaminen pankista on maksullinen lisäpalvelu hintaan %1 kuukaudessa (sis.alv), mikä veloitetaan jälkikäteen.\n\n"
                                  "Palvelua voi ensin kokeilla maksutta %2 päivän ajan. Kokeilujakso alkaa ensimmäisestä onnistuneesta tilitapahtumien hakemisesta. "
                                  "Maksu on kirjanpitokohtainen, ja veloitetaan niiltä kuukausilta, jolloin tilitietoja on haettu onnistuneesti."
                                  "\n\nKatso lisätietoja Kitsaan ohjeista!").arg(palvelu_->price().display()).arg(palvelu_->trialDays()));        
    }

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

    if( ui->stackedWidget->currentIndex() == VALITSEPANKKI) {
        pankkiValittu();
    } else {
        ui->seuraavaNappi->setEnabled(false);
    }


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
    if( ui->stackedWidget->currentIndex() == MAKSUINFO) {
        pankkiValittu();
        kp()->asetukset()->aseta(AsetusModel::TilitietoMaksuHyvaksytty, QDateTime::currentDateTime().toString("yyyy-MM-dd"));
        ui->stackedWidget->setCurrentIndex(VALITSEPANKKI);
    } else if( ui->stackedWidget->currentIndex() == VALITSEPANKKI) {
        ui->seuraavaNappi->setEnabled(false);
        int pankki = ui->pankkiView->currentIndex().data(PankitModel::IdRooli).toInt();
        if( pankki ) {
            palvelu_->lisaaValtuutus(pankki);
        }
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
