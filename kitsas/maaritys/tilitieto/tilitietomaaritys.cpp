#include "tilitietomaaritys.h"

#include "lisaikkuna.h"

#include "ui_tilitietomaaritys.h"
#include "tilitietopalvelu.h"
#include "uusiyhteysdialog.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pankkilokimodel.h"

#include "tilitapahtumahakudialog.h"

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

    ui->lokiView->setModel( palvelu_->loki() );

    connect( ui->lisaaNappi, &QPushButton::clicked, dlg_, &UusiYhteysDialog::lisaaValtuutus);
    connect( palvelu_, &TilitietoPalvelu::ladattu, this, &TilitietoMaaritys::paivitaTilaus);
    connect( palvelu_, &TilitietoPalvelu::ladattu, this, &TilitietoMaaritys::paivitaYhteydet);
    connect( ui->lokiView, &QTableView::clicked, this, &TilitietoMaaritys::naytaTosite);
}

TilitietoMaaritys::~TilitietoMaaritys()
{
    delete ui;
    delete dlg_;
}

bool TilitietoMaaritys::nollaa()
{
    palvelu_->lataa();
    return true;
}

void TilitietoMaaritys::paivitaTilaus()
{

    Euro price = palvelu_->price();
    const int kokeiluPituus = palvelu_->trialDays();
    const QDate kokeiluSaakka = palvelu_->trialPeriod();

    ui->infoLabel->setVisible( price  );

    const QString maksutieto = tr("Tilitietojen hakeminen on maksullinen lisäpalvelu hintaan %1/kk (sis.alv).").arg(price.display());

    if( kokeiluSaakka.isNull()) {
        ui->infoLabel->setText(  maksutieto + "\n" + tr("Voit kokeilla palvelua maksutta %1 päivän ajan.").arg(kokeiluPituus) );
    } else if( kokeiluSaakka <= QDate::currentDate() ) {
        ui->infoLabel->setText( maksutieto + "\n" + tr("Hakeminen on maksutonta vielä kokeilujakson ajan %1 saakka.").arg(kokeiluSaakka.toString("dd.MM.yyyy")));
    } else {
        ui->infoLabel->setText( maksutieto );
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

            int voimassaPaivaa = QDateTime::currentDateTime().daysTo(voimassa);
            if( voimassaPaivaa < 0) {
                vLabel->setStyleSheet("font-weight: bold; color: red");
            } else if( voimassaPaivaa < 7) {
                vLabel->setStyleSheet("font-weight: bold; background-color: orange");
            } else if( voimassaPaivaa < 14) {
                vLabel->setStyleSheet("font-weight: bold; background-color: yellow");
            } else if( voimassaPaivaa < 21) {
                vLabel->setStyleSheet("background-color: greenyellow");
            }

        }

        int yhdistettyjaTileja = 0;

        for( int j = 0; j < yhteys.tileja(); j++) {
            YhdistettyTili tili = yhteys.tili(j);
            QLabel *iban = new QLabel( tili.iban().valeilla());
            apuleiska->addWidget(iban, j+1, 0);

            Tili kirjanpitoTili = kp()->tilit()->tiliIbanilla(tili.iban().valeitta());

            if( kirjanpitoTili.onkoValidi()) {
                yhdistettyjaTileja++;
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

        if( yhdistettyjaTileja && yhteys.voimassa() > QDateTime::currentDateTime() ) {
            QPushButton* haeNappi = new QPushButton(QIcon(":/pic/down.png"), tr("Nouda tapahtumia"));
            connect( haeNappi, &QPushButton::clicked, [i, this] { this->haeTiliTapahtumat(i);});
            nappiLeiska->addWidget(haeNappi);
        }

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

    ui->lokiView->resizeColumnsToContents();
}

void TilitietoMaaritys::naytaTosite(const QModelIndex &index)
{
    const int tositeId = index.data(PankkiLokiModel::DocumentIdRole).toInt();
    if( tositeId ) {
        LisaIkkuna* ikkuna = new LisaIkkuna();
        ikkuna->naytaTosite(tositeId);
    }
}

void TilitietoMaaritys::haeTiliTapahtumat(int yhteysIndeksi)
{
    TiliTapahtumaHakuDialog dialog( palvelu_, this );
    dialog.nayta(yhteysIndeksi);
}

} // namespace
