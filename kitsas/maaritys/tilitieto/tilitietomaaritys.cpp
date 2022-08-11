#include "tilitietomaaritys.h"

#include "ui_tilitietomaaritys.h"
#include "tilitietopalvelu.h"
#include "uusiyhteysdialog.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QHBoxLayout>

namespace Tilitieto {

TilitietoMaaritys::TilitietoMaaritys() :
    ui(new Ui::TilitietoMaaritys),
    palvelu_( kp()->pilvi()->tilitietoPalvelu() ),
    dlg_(new UusiYhteysDialog(palvelu_))
{
    ui->setupUi(this);

    yhteysLeiska_ = new QHBoxLayout(this);
    ui->yhteydetWidget->setLayout(yhteysLeiska_);

    palvelu_->lataa();

    connect( ui->lisaaNappi, &QPushButton::clicked, dlg_, &UusiYhteysDialog::lisaaValtuutus);
    connect( palvelu_, &TilitietoPalvelu::ladattu, this, &TilitietoMaaritys::nollaa);
}

TilitietoMaaritys::~TilitietoMaaritys()
{
    delete ui;
    delete dlg_;
}

bool TilitietoMaaritys::nollaa()
{
    paivitaTilaus();
    paivitaYhteydet();

    return true;
}

void TilitietoMaaritys::paivitaTilaus()
{

    int trial = palvelu_->trialPeriod();
    Euro price = palvelu_->price();

    if( trial ) {
        ui->infoLabel->setText( tr("Voit kokeilla tilitietojen hakemista maksutta vielä %1 päivän ajan, "
                                   "minkä jälkeen palvelu maksaa %2 kuukaudessa (sis. alv)").arg(trial).arg(price.display()));
        ui->infoLabel->setStyleSheet("background-color: rgba(255, 255, 0, 125)");

    } else {
        ui->infoLabel->setText( tr("Tilitietojen hakeminen on maksullinen lisäpalvelu, hinta %1 kuukaudessa (sis. alv)").arg(price.display()) );
        ui->infoLabel->setStyleSheet("");
    }

}

void TilitietoMaaritys::paivitaYhteydet()
{
    QGridLayout* paaleiska = new QGridLayout(this);
    for(int i=0; i < palvelu_->yhteyksia(); i++) {
        Yhteys yhteys = palvelu_->yhteys(i);
        Pankki pankki = yhteys.pankki();
        if( !pankki.id()) break;

        QLabel* logo = new QLabel();
        QImage image = pankki.logo();
        QPixmap pix = QPixmap::fromImage( image.scaled(128,128) );
        logo->setPixmap( pix );
        paaleiska->addWidget(logo, i, 0);

        QGridLayout* apuleiska = new QGridLayout(this);
        QLabel* pankkiNimi = new QLabel( pankki.nimi() );
        QFont lihava;
        lihava.setBold(true);
        pankkiNimi->setFont(lihava);
        apuleiska->addWidget(pankkiNimi,0, 0);

        QDateTime voimassa = yhteys.voimassa();
        if( voimassa.isValid()) {
            QLabel* vLabel = new QLabel( (tr("Valtuutus voimassa %1 saakka").arg(voimassa.toLocalTime().toString("dd.MM.yyyy"))) );
            apuleiska->addWidget(vLabel,0, 1, 1, 1);
        }

        for( int j = 0; j < yhteys.tileja(); j++) {
            YhdistettyTili tili = yhteys.tili(j);
            QLabel *iban = new QLabel( tili.iban().valeilla());
            apuleiska->addWidget(iban, j+1, 0);

            Tili kirjanpitoTili = kp()->tilit()->tiliIbanilla(tili.iban().valeitta());

            if( kirjanpitoTili.onkoValidi()) {
                QLabel *tlabel = new QLabel( kirjanpitoTili.nimiNumero() );
                apuleiska->addWidget(tlabel, j+1, 1);

                if( tili.paivitetty().isValid()) {
                    QLabel *plabel = new QLabel(tr("Haettu %1").arg(tili.paivitetty().toLocalTime().toString("dd.MM.yyyy")));
                    plabel->setWordWrap(true);
                    apuleiska->addWidget(plabel, j+1, 2);
                }
            } else {
                QLabel* el = new QLabel(tr("IBAN-numeroa ei tililuettelossa"));
                el->setStyleSheet("color: red");
                apuleiska->addWidget(el, j+1, 1, 1, 2);
            }

        }
        paaleiska->addLayout(apuleiska, i, 1);

        QVBoxLayout* nappiLeiska = new QVBoxLayout;
        QPushButton* paivita = new QPushButton(QIcon(":/pic/refresh.png"), tr("Uudista valtuutus"));        
        int pankkiId = pankki.id();

        connect( paivita, &QPushButton::clicked, [ pankkiId, this] { this->palvelu_->lisaaValtuutus(pankkiId);} );
        nappiLeiska->addWidget(paivita);

        QPushButton* poista = new QPushButton(QIcon(":/pic/poista.png"), tr("Poista"));
        connect( poista, &QPushButton::clicked, [this, pankkiId] { this->palvelu_->poistaValtuutus(pankkiId); } );
        nappiLeiska->addWidget(poista);

        paaleiska->addLayout(nappiLeiska, i, 2);

    }
    paaleiska->setColumnStretch(0,0);
    paaleiska->setColumnStretch(1,4);
    paaleiska->setColumnStretch(2,1);

    if( yhteysWidget_ ) {
        yhteysLeiska_->removeWidget(yhteysWidget_);
        yhteysWidget_->deleteLater();
    }
    yhteysWidget_ = new QWidget(this);
    yhteysWidget_->setLayout(paaleiska);
    yhteysLeiska_->addWidget(yhteysWidget_);
}

} // namespace
