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

#include "tilinmuokkausdialog.h"
#include "db/tilimodel.h"
#include "db/tilinvalintaline.h"
#include "validator/ibanvalidator.h"
/*
TilinMuokkausDialog::TilinMuokkausDialog(TiliModel *model, QModelIndex index) :
    QDialog(nullptr), model_(model), index_(index)
{
    ui = new Ui::tilinmuokkausDialog();
    ui->setupUi(this);





    // Laitetaa verotyypit paikalleen

    veroproxy_ = new QSortFilterProxyModel(this);
    veroproxy_->setSourceModel( kp()->alvTyypit());
    veroproxy_->setFilterRole( VerotyyppiModel::KoodiRooli);
    ui->veroCombo->setModel( veroproxy_ );

    ui->poistotiliEdit->asetaModel( model );
    ui->poistotiliEdit->suodataTyypilla("DP");

    ui->ibanLabel->hide();
    ui->ibanLine->hide();
    ui->ibanLine->setValidator(new IbanValidator());


    connect( ui->numeroEdit, SIGNAL(textChanged(QString)), this, SLOT(nroMuuttaaTyyppia(QString)));

    connect( ui->ibanLine, SIGNAL(textEdited(QString) ), this, SLOT( ibanCheck()) );


    // Tallennusnappi ei käytössä ennen kuin tiedot kunnossa
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);

    naytettavienPaivitys();

    if( index.isValid())
        lataa();

}

*/

TilinMuokkausDialog::TilinMuokkausDialog(QWidget *parent, int indeksi, Tila tila)
    : QDialog(parent), ui(new Ui::tilinmuokkausDialog), tila_(tila)
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


    if( tila == UUSITILI || tila == UUSIOTSIKKO ) {
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

        for(int i=indeksi; i < kp()->tilit()->rowCount(); i++) {
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
        ui->numeroEdit->setText( minNumero_ );
        ui->numeroEdit->setReadOnly(true);

    }

    // Nimitaulu
    alustaNimet();

    if( minNumero_.startsWith("1"))
        proxy_->setFilterFixedString("A");
    else if( minNumero_.startsWith("2"))
        proxy_->setFilterRegularExpression("[BT]");
    else
        proxy_->setFilterRegExp("[CD]");

    alustaOhjeet();

    if( tila_ == MUOKKAATILI || tila_ == MUOKKAAOTSIKKO)
        lataa();

    naytettavienPaivitys();


}

TilinMuokkausDialog::~TilinMuokkausDialog()
{
    delete ui;
}

void TilinMuokkausDialog::lataa()
{
    Tili tili = model_->tiliIndeksilla( index_.row());

    // Ei voi muuttaa otsikkoa tiliksi tai päin vastoin

    ui->ibanLabel->setVisible( tili.onko(TiliLaji::PANKKITILI));
    ui->ibanLine->setVisible( tili.onko(TiliLaji::PANKKITILI));
    ui->ibanLine->setText(tili.str("IBAN"));

    proxy_->setFilterRegExp("");
    ui->tyyppiCombo->setCurrentIndex( ui->tyyppiCombo->findData( tili.tyyppiKoodi()) );

    ui->veroSpin->setValue( tili.luku("AlvProsentti"));

    int alvlaji = tili.luku("AlvLaji");
    ui->veroCombo->setCurrentIndex( ui->veroCombo->findData( alvlaji , VerotyyppiModel::KoodiRooli) );

    ui->poistoaikaSpin->setValue( tili.luku("Tasaerapoisto") / 12);  // kk -> vuosi
    ui->poistoprossaSpin->setValue( tili.luku("Menojaannospoisto"));

    int taseEraValinta = tili.luku("Taseerittely");
    ui->taseEratRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_TAYSI);
    ui->taseEraLuettelo->setChecked( taseEraValinta == Tili::TASEERITTELY_LISTA);
    ui->teLiVaRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_MUUTOKSET);
    ui->teSaldoRadio->setChecked( taseEraValinta == Tili::TASEERITTELY_SALDOT);

    ui->kohdennusCheck->setChecked( tili.luku("Kohdennukset") );


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
    ui->ohjeLabel->setVisible(!taso_);
    ui->ohjeTabs->setVisible(!taso_);

    // Ellei alv-toimintoja käytettävissä, ne piilotetaan
    bool alvKaytossa = ( tyyppi.onko(TiliLaji::TULOS) || tyyppi.onko(TiliLaji::POISTETTAVA));

    ui->verolajiLabel->setVisible( alvKaytossa );
    ui->veroCombo->setVisible( alvKaytossa );
    ui->veroprosenttiLabel->setVisible( alvKaytossa );
    ui->veroSpin->setVisible( alvKaytossa);

    if( tyyppi.onko(TiliLaji::TULO)) {
        veroproxy_->setFilterRegExp("^(0|1[1-7].)$");
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

    ui->laskullaGroup->setVisible( tyyppi.onko(TiliLaji::PANKKITILI));

    ui->kohdennusCheck->setVisible( tyyppi.onko(TiliLaji::TASE));

    // #46 Alv-velka ja alv-saatava -tileille ei voi tehdä tase-erittelyä, koska tilit tyhjennetään aina
    // kuukauden lopussa alv-kirjauksella, joka ei huomioi tase-eriä

    ui->teGroup->setVisible( tyyppi.onko(TiliLaji::TASE) && !tyyppi.onko(TiliLaji::ALVSAATAVA) && !tyyppi.onko(TiliLaji::ALVVELKA));

    maksutapaPaivitys();
    palkkalajiPaivitys();
    veroEnablePaivita();

}

void TilinMuokkausDialog::maksutapaPaivitys()
{
    QString maksutapa = ui->maksutapaCombo->currentData().toString();
    ui->maksutapaCombo->clear();
    ui->maksutapaCombo->addItem("");

    TiliTyyppi tyyppi = kp()->tiliTyypit()->tyyppiKoodilla( ui->tyyppiCombo->currentData().toString() );
    if( tyyppi.onko(TiliLaji::KATEINEN))
        ui->maksutapaCombo->addItem(QIcon(":/pic/kateinen.png"),tr("Käteinen"),"KA");
    if( tyyppi.onko(TiliLaji::PANKKITILI))
        ui->maksutapaCombo->addItem(QIcon(":/pic/pankki.png"),tr("Pankkitili"),"PA");
    if( tyyppi.onko(TiliLaji::MYYNTISAATAVA))
        ui->maksutapaCombo->addItem(QIcon(":/pic/lasku.png"),tr("Lasku, tuloa"),"LA+");
    if( tyyppi.onko(TiliLaji::OSTOVELKA))
        ui->maksutapaCombo->addItem(QIcon(":/pic/lasku.png"),tr("Lasku, menoa"),"LA-");
    if( tyyppi.koodi() == "AS")
        ui->maksutapaCombo->addItem(QIcon(":/pic/luottokortti.png"),tr("Luottokortti, tuloa"),"LU+");
    if( tyyppi.koodi() == "BS")
        ui->maksutapaCombo->addItem(QIcon(":/pic/luottokortti.png"),tr("Luottokortti, menoa"),"LU-");

    ui->maksutapaLabel->setVisible( ui->maksutapaCombo->count() > 1 );
    ui->maksutapaCombo->setVisible( ui->maksutapaCombo->count() > 1);
    ui->maksutapaCombo->setCurrentIndex( ui->maksutapaCombo->findData(maksutapa) );

}

void TilinMuokkausDialog::palkkalajiPaivitys()
{
    QString palkkalaji = ui->palkkaCombo->currentData().toString();
    ui->palkkaCombo->clear();
    ui->palkkaCombo->addItem("");

    TiliTyyppi tyyppi = kp()->tiliTyypit()->tyyppiKoodilla( ui->tyyppiCombo->currentData().toString() );
    if( tyyppi.koodi() == "AS") {
        ui->palkkaCombo->addItem(tr("Eläkemaksuvelka"),"EV");
        ui->palkkaCombo->addItem(tr("Ay-maksuvelka"),"AY");
    } else if( tyyppi.onko(TiliLaji::VEROVELKA) )
        ui->palkkaCombo->addItem(tr("Ennakonpidätys- ja sairausvakuutusmaksuvelka"),"VV");
    else if( tyyppi.onko(TiliLaji::MENO)) {
        ui->palkkaCombo->addItem(tr("Palkat"),"PA");
        ui->palkkaCombo->addItem(tr("Palkkiot"),"PI");
        ui->palkkaCombo->addItem(tr("Lisät ja korvaukset"),"LI");
        ui->palkkaCombo->addItem(tr("Loma-ajan ja sosiaalipalkat"),"LS");
        ui->palkkaCombo->addItem(tr("Luontoisedut"),"LU");
        ui->palkkaCombo->addItem(tr("Luontoisetujen vastatili"),"LV");
        ui->palkkaCombo->addItem(tr("Kilometrikorvaukset"),"KM");
        ui->palkkaCombo->addItem(tr("Päivärahat"),"PR");
        ui->palkkaCombo->addItem(tr("Ateriakorvaukset"),"AT");
        ui->palkkaCombo->addItem(tr("Eläkevakuutusmaksut"),"EL");
        ui->palkkaCombo->addItem(tr("Työttömyysvakuutusmaksut"),"TV");
    }
    ui->palkkaLabel->setVisible( ui->palkkaCombo->count() > 1 );
    ui->palkkaCombo->setVisible( ui->palkkaCombo->count() > 1);
    ui->palkkaCombo->setCurrentIndex( ui->palkkaCombo->findData(palkkalaji));
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
    ui->buttonBox->setFocus();

    // Kaikki kunnossa eli voidaan tallentaa modeliin
    QString tyyppikoodi = ui->tyyppiCombo->currentData().toString();

    TiliTyyppi tilityyppi = kp()->tiliTyypit()->tyyppiKoodilla(tyyppikoodi);




    QDialog::accept();
}

void TilinMuokkausDialog::alustaNimet()
{
    for(QString kieli : kp()->asetukset()->kielet()) {
        QListWidgetItem* item = new QListWidgetItem( lippu(kieli), tili_ ? tili_->nimi(kieli) : "", ui->nimiList  );
        item->setData(Qt::UserRole, kieli);
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }
}

void TilinMuokkausDialog::alustaOhjeet()
{
    for( QString kieli : kp()->asetukset()->kielet()) {
        QPlainTextEdit* edit = new QPlainTextEdit;
        if( tili_ )
            edit->setPlainText( tili_->ohjeKaannos(kieli) );
        ui->ohjeTabs->addTab(edit,lippu(kieli),kp()->asetukset()->kieli(kieli));
    }
}




Tili TilinMuokkausDialog::ylaotsikko(int ysinro)
{
    Tili ylatili;

    for( int i=0; i < model_->rowCount(QModelIndex()) ; i++)
    {
       Tili tili = model_->tiliIndeksilla(i);
    }
    return ylatili;
}

