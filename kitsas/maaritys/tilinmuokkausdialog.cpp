/*
   Copyright (C) 2017,2018 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QMapIterator>
#include <QIntValidator>

#include <QDebug>
#include <QPushButton>
#include <QMessageBox>
#include <QIntValidator>
#include <QPlainTextEdit>
#include <QJsonDocument>
#include <QVariant>
#include <QScreen>

#include "tilinmuokkausdialog.h"
#include "db/tilimodel.h"
#include "db/tilinvalintaline.h"
#include "validator/ibanvalidator.h"


TilinMuokkausDialog::TilinMuokkausDialog(QWidget *parent, int indeksi, Tila tila)
    : QDialog(parent), ui(new Ui::tilinmuokkausDialog),  tila_(tila), indeksi_(indeksi)
{
    ui->setupUi(this);

    proxy_ = new QSortFilterProxyModel(this);
    proxy_->setSourceModel( kp()->tiliTyypit() );
    proxy_->setFilterRole(TilityyppiModel::KoodiRooli);
    ui->tyyppiCombo->setModel( proxy_ );
    ui->numeroEdit->setValidator( new QIntValidator(0,999999999,this));
    veroproxy_ = new QSortFilterProxyModel(this);
    veroproxy_->setSourceModel( kp()->alvTyypit());
    veroproxy_->setFilterRole( VerotyyppiModel::KoodiRooli);
    ui->veroCombo->setModel( veroproxy_ );

    ui->poistotiliCombo->suodataTyypilla("DP", true);
    ui->ibanLine->setValidator(new IbanValidator());    

    alustalaajuus();

    connect( ui->tyyppiCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(tarkasta()));
    connect( ui->veroCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(veroEnablePaivita()));
    connect( ui->ibanLine, SIGNAL(textEdited(QString) ), this, SLOT( ibanCheck()) );
    connect( ui->numeroEdit, &QLineEdit::textEdited, this, &TilinMuokkausDialog::numeroCheck);
    connect( ui->vastaCombo, &TiliCombo::tiliValittu, [this] (int tili) { ui->vastaCheck->setChecked(tili); } );
    connect( ui->vastaCheck, &QCheckBox::clicked, [this] (bool tila) { if(!tila) ui->vastaCombo->setCurrentIndex(-1); } );

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("maaritykset/tilikartta");});

    if( tila == UUSITILI || tila == UUSIOTSIKKO ) {

        // VANHEMMAN ETSIMINEN FOR-SILMUKALLA, JOTTA
        // ONNISTUU VAIKKA TAMANOTSIKKO EI TOIMI !!!

        vanhempi_ = kp()->tilit()->tiliPIndeksilla(indeksi);
        if( !vanhempi_->otsikkotaso())
            vanhempi_ = vanhempi_->tamanOtsikko();

        if( tila == UUSIOTSIKKO ) {
            taso_ = vanhempi_->otsikkotaso() + 1;
            setWindowTitle(tr("Uusi otsikko"));
        } else {
            setWindowTitle(tr("Uusi tili"));
        }

        minNumero_ = QString::number(vanhempi_->numero());
        int oletusnumero = kp()->tilit()->tiliPIndeksilla(indeksi)->numero();

        while( oletusnumero / 1000 < 1)
            oletusnumero *= 10;

        for(int i=indeksi+1; i < kp()->tilit()->rowCount(); i++) {
            Tili* tamatili = kp()->tilit()->tiliPIndeksilla(i);
            if( tamatili->numero() == oletusnumero )
                oletusnumero++;
            if( !tamatili->otsikkotaso())
                ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tamatili->tyyppiKoodi() ) );
            if( tamatili->otsikkotaso() && tamatili->otsikkotaso() <= vanhempi_->otsikkotaso()) {
                maxnumero_ = QString::number(tamatili->numero());
                break;
            }
        }

        // Näytetään tilin numeroväli nelinumeroisena

        int min = minNumero_.toInt();
        while( min / 1000 < 1)
            min*=10;
        int max = maxnumero_.toInt();
        if( !max)
            max = 9999;
        while( max / 1000 < 1)
            max*=10;
        max--;

        ui->infoLabel->setText( QString("%1 [%2 .. %3]").arg(vanhempi_->nimi()).arg(min).arg(max));
        ui->numeroEdit->setText( QString::number(oletusnumero) );
        numeroCheck();
    } else {
        tili_ = kp()->tilit()->tiliPIndeksilla(indeksi);
        minNumero_ = QString::number( tili_->numero());
        taso_ = tili_->otsikkotaso();
        ui->numeroEdit->setText( minNumero_ );
        ui->numeroEdit->setReadOnly(true);
        if( taso_)
            setWindowTitle(tr("Otsikon muokkaus"));
        if( tili_->tamanOtsikko())
            ui->infoLabel->setText( tili_->tamanOtsikko()->nimi() );
        else
            ui->infoLabel->hide();

        poistaNappi_ = new QPushButton(QIcon(":/pic/roskis.png"), tr("Poista"));
        poistaNappi_->setEnabled(false);
        ui->buttonBox->addButton( poistaNappi_, QDialogButtonBox::DestructiveRole);
        connect( poistaNappi_, &QPushButton::clicked, this, &TilinMuokkausDialog::poista );

        if( taso_) {
            // Otsikon saa poistaa, jos sillä ei ole alempia otsikoita
            if( taso_ > 1 &&
                kp()->tilit()->tiliPIndeksilla(indeksi+1) &&
                kp()->tilit()->tiliPIndeksilla(indeksi+1)->otsikkotaso() <= taso_)
                poistaNappi_->setEnabled(true);
        } else {
            // Tilin saa poistaa, jos sillä ei ole vientejä
            KpKysely *kysely = kpk("/viennit");
            kysely->lisaaAttribuutti("tili", tili_->numero());
            connect( kysely, &KpKysely::vastaus, this, &TilinMuokkausDialog::viennitSaapuu );
            kysely->kysy();
        }

    }

    // Nimitaulu
    alustaNimet();

    if( minNumero_.startsWith("1"))
        proxy_->setFilterFixedString("A");
    else if( minNumero_.startsWith("2"))
        proxy_->setFilterRegExp("[BT]");
    else
        proxy_->setFilterRegExp("[CD]");

    alustaOhjeet();

    if( tila_ == MUOKKAA)
        lataa();

    naytettavienPaivitys();    
    setMaximumHeight( qApp->screens()[0]->size().height() * 4 / 5 );
}

TilinMuokkausDialog::~TilinMuokkausDialog()
{
    delete ui;
}

void TilinMuokkausDialog::lataa()
{
    proxy_->setFilterRegExp("");
    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili_->tyyppiKoodi()) );


    ui->ibanLine->setText(tili_->str("IBAN"));
    ui->veroSpin->setValue( tili_->luku("alvprosentti"));

    int alvlaji = tili_->luku("alvlaji");
    ui->veroCombo->setCurrentIndex( ui->veroCombo->findData( alvlaji , VerotyyppiModel::KoodiRooli) );

    ui->poistoaikaSpin->setValue( tili_->luku("tasaerapoisto") / 12);  // kk -> vuosi
    ui->poistoprossaSpin->setValue( tili_->luku("menojaannospoisto"));
    ui->poistotiliCombo->valitseTili( tili_->luku("poistotili") );
    ui->jaksotiliCombo->valitseTili( tili_->luku("jaksotustili"));

    int taseEraValinta = tili_->luku("erittely");
    ui->taseEratRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_TAYSI);
    ui->taseEraLuettelo->setChecked( taseEraValinta == Tili::TASEERITTELY_LISTA);
    ui->teLiVaRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_MUUTOKSET);
    ui->teSaldoRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_SALDOT);

    int vastatili = tili_->luku("vastatili");
    ui->vastaCheck->setChecked( vastatili );
    ui->vastaCombo->valitseTili(vastatili);

    ui->laajuusCombo->setCurrentIndex( ui->laajuusCombo->findData( tili_->laajuus() ) );


}

void TilinMuokkausDialog::veroEnablePaivita()
{
    // Jos veroton, niin eipä silloin laiteta alv-prosenttia
    if( ui->veroCombo->currentData(VerotyyppiModel::NollaLajiRooli).toBool() || ui->veroCombo->isHidden())
    {
       ui->veroSpin->setVisible(false);
       ui->veroprosenttiLabel->setVisible(false);
    }
    else
    {
        ui->veroSpin->setVisible(true);
        ui->veroprosenttiLabel->setVisible(true);
        // Lisäksi laitetaan oletusvero jo nolla
        if( ui->veroSpin->value() == 0)
            ui->veroSpin->setValue( VerotyyppiModel::oletusAlvProsentti());
    }

}


void TilinMuokkausDialog::naytettavienPaivitys()
{

    TiliTyyppi tyyppi = kp()->tiliTyypit()->tyyppiKoodilla( ui->tyyppiCombo->currentData().toString() );
    if( taso_ )
        tyyppi = TiliLaji::OTSIKKO;

    ui->laajuusLabel->setVisible( !taso_ );
    ui->laajuusCombo->setVisible( !taso_ );
    ui->tyyppiCombo->setVisible( !taso_ );
    ui->tyyppiLabel->setVisible( !taso_ );    
    ui->tabWidget->setTabEnabled(OHJE, !taso_);
    ui->tabWidget->setTabEnabled(ERITTELY, tyyppi.onko(TiliLaji::TASE) && !tyyppi.onko(TiliLaji::ALVSAATAVA) && !tyyppi.onko(TiliLaji::ALVVELKA));
    ui->tabWidget->setTabEnabled(POISTO, tyyppi.onko(TiliLaji::POISTETTAVA));

    // Ellei alv-toimintoja käytettävissä, ne piilotetaan
    bool alvKaytossa = ( tyyppi.onko(TiliLaji::TULOS) || tyyppi.onko(TiliLaji::POISTETTAVA));
    ui->tabWidget->setTabEnabled( ALV, alvKaytossa );

    if( tyyppi.onko(TiliLaji::TULO)) {
        veroproxy_->setFilterRegExp("^(0|1[1-7])$");
        ui->jaksotiliCombo->suodataTyypilla("AJ", true);
    } else if( tyyppi.onko(TiliLaji::MENO)) {
        veroproxy_->setFilterRegExp("^(0|2[1-7])$");
        ui->jaksotiliCombo->suodataTyypilla("BJ", true);
    } else
        veroproxy_->setFilterRegExp("");

    ui->ibanLabel->setVisible( tyyppi.onko(TiliLaji::PANKKITILI));
    ui->ibanLine->setVisible( tyyppi.onko(TiliLaji::PANKKITILI));

    ui->poistoaikaLabel->setVisible( tyyppi.onko( TiliLaji::TASAERAPOISTO));
    ui->poistoaikaSpin->setVisible( tyyppi.onko(TiliLaji::TASAERAPOISTO) );

    ui->poistoprossaLabel->setVisible( tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO));
    ui->poistoprossaSpin->setVisible( tyyppi.onko(TiliLaji::MENOJAANNOSPOISTO ));

    ui->poistotiliLabel->setVisible( tyyppi.onko(TiliLaji::POISTETTAVA));
    ui->poistotiliCombo->setVisible( tyyppi.onko(TiliLaji::POISTETTAVA));

    ui->jaksotusLabel->setVisible( tyyppi.onko(TiliLaji::TULOS) && !tyyppi.onko(TiliLaji::POISTO));
    ui->jaksotiliCombo->setVisible( tyyppi.onko(TiliLaji::TULOS) && !tyyppi.onko(TiliLaji::POISTO));
    ui->vastaCheck->setVisible(tyyppi.onko(TiliLaji::TULOS) && !tyyppi.onko(TiliLaji::POISTO) );
    ui->vastaCombo->setVisible(tyyppi.onko(TiliLaji::TULOS) && !tyyppi.onko(TiliLaji::POISTO));

    veroEnablePaivita();

}


void TilinMuokkausDialog::alustalaajuus()
{
    QVariantMap laajamap = QJsonDocument::fromJson( kp()->asetus("laajuudet").toUtf8() ).toVariant().toMap();
    QMapIterator<QString,QVariant> iter(laajamap);
    while (iter.hasNext()) {
        iter.next();
        KieliKentta kk(iter.value());
        ui->laajuusCombo->addItem( kk.teksti(), iter.key() );
    }
    ui->laajuusCombo->setCurrentIndex( ui->laajuusCombo->findData( kp()->asetus("laajuus") ) );
}



void TilinMuokkausDialog::tarkasta()
{

   naytettavienPaivitys();

}

void TilinMuokkausDialog::ibanCheck()
{
    switch ( IbanValidator::kelpo( ui->ibanLine->text())) {
    case IbanValidator::Acceptable:
        ui->ibanLine->setStyleSheet("color: darkGreen;");
        break;
    case IbanValidator::Invalid :
        ui->ibanLine->setStyleSheet("color: red;");
        break;
    default:
        ui->ibanLine->setStyleSheet("color: black;");
        break;
    }

}

void TilinMuokkausDialog::numeroCheck()
{
    if( tila_ == UUSITILI || tila_ == UUSIOTSIKKO) {
        QString numero = ui->numeroEdit->text();
        bool numerokelpaa =  !kp()->tilit()->tiliNumerolla(numero.toInt(), taso_).onkoValidi() &&
                numero >= minNumero_ &&
                numero < maxnumero_;
        ui->numeroEdit->setStyleSheet( numerokelpaa ? "color: black;" : "color: red;" );
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(numerokelpaa);
    }

}

void TilinMuokkausDialog::accept()
{
    if( !tili_)
        tili_ = kp()->tilit()->lisaaTili( ui->numeroEdit->text().toInt(), taso_ );

    if( !taso_) {
        QString tyyppi = ui->tyyppiCombo->currentData().toString();
        tili_->asetaTyyppi( tyyppi );
        tili_->asetaLaajuus( ui->laajuusCombo->currentData().toInt());

        tili_->setInt("alvlaji", ui->veroCombo->currentData().toInt());
        if( ui->veroSpin->isVisible())
            tili_->setInt("alvprosentti", ui->veroSpin->value());
        else
            tili_->unset("alvprosentti");
        tili_->setInt("tasaerapoisto", tyyppi == "APT" ? ui->poistoaikaSpin->value() * 12 : 0);
        tili_->setInt("menojaannospoisto", tyyppi == "APM" ? ui->poistoprossaSpin->value() : 0);
        tili_->setInt("poistotili", tyyppi.startsWith("AP") ?  ui->poistotiliCombo->valittuTilinumero() : 0);
        if( !ui->vastaCheck->isChecked())
            tili_->unset("vastatili");
        else
            tili_->setInt("vastatili", ui->vastaCombo->valittuTilinumero());

        if( tili_->onko(TiliLaji::TULOS)) {
            tili_->setInt("jaksotustili", ui->jaksotiliCombo->valittuTilinumero());
            tili_->unset("erittely");
        } else {
            if( ui->taseEratRadio->isChecked() )
                tili_->setInt("erittely", Tili::TASEERITTELY_TAYSI);
            else if( ui->taseEraLuettelo->isChecked())
                tili_->setInt("erittely", Tili::TASEERITTELY_LISTA);
            else if( ui->teLiVaRadio->isChecked())
                tili_->setInt("erittely", Tili::TASEERITTELY_MUUTOKSET);
            else if( ui->teSaldoRadio->isChecked())
                tili_->setInt("erittely", Tili::TASEERITTELY_SALDOT);
        }
    }

    if( tili_->onko(TiliLaji::PANKKITILI) && IbanValidator::kelpaako( ui->ibanLine->text()) )
        tili_->set("IBAN", ui->ibanLine->text().remove(QRegularExpression("\\W")));

    for(int i=0; i < ui->nimiList->count(); i++)
        tili_->asetaNimi( ui->nimiList->item(i)->text(), ui->nimiList->item(i)->data(Qt::UserRole).toString() );
    for(int i=0; i < ui->ohjeTabs->count(); i++) {
        QPlainTextEdit *edit = qobject_cast<QPlainTextEdit*>( ui->ohjeTabs->widget(i) );
        tili_->asetaOhje( edit->toPlainText(), edit->property("Kielikoodi").toString() );
    }    

    kp()->tilit()->tallenna(tili_);

    QDialog::accept();
}

void TilinMuokkausDialog::poista()
{
    if( QMessageBox::question(this, tr("Vahvista poisto"),
                              tr("Haluatko varmasti poistaa tämän tilin? Tarpeeton tili on yleensä "
                                 "suositeltavampaa piilottaa kuin poistaa.")) == QMessageBox::Yes)
    {
        kp()->tilit()->poistaRivi(indeksi_);
        QDialog::accept();
    }
}

void TilinMuokkausDialog::viennitSaapuu(QVariant *data)
{
    poistaNappi_->setEnabled( data->toList().isEmpty() );
}


void TilinMuokkausDialog::alustaNimet()
{
    for(QString kieli : kp()->asetukset()->kielet()) {        
        QListWidgetItem* item = new QListWidgetItem( lippu(kieli), tili_ ? tili_->nimiKaannos(kieli) : "" , ui->nimiList  );
        item->setData(Qt::UserRole, kieli);
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }
}

void TilinMuokkausDialog::alustaOhjeet()
{
    for( QString kieli : kp()->asetukset()->kielet()) {
        QPlainTextEdit* edit = new QPlainTextEdit;
        if( tili_ ) {
            edit->setPlainText( tili_->ohjeKaannos(kieli) );            
        }
        edit->setProperty("Kielikoodi", kieli);
        ui->ohjeTabs->addTab(edit,lippu(kieli),kp()->asetukset()->kieli(kieli));
    }
}


