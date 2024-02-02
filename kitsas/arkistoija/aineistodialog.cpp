/*
   Copyright (C) 2019 Arto Hyvättinen

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
#include "aineistodialog.h"
#include "ui_aineistodialog.h"

#include "db/kirjanpito.h"
#include <QSettings>
#include <QFileDialog>

#include <QPdfWriter>
#include <QPainter>
#include <QApplication>
#include <QDesktopServices>
#include <QProgressDialog>
#include <QDebug>
#include <QPdfWriter>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QMessageBox>

#include <QDir>
#include <QPdfDocument>
#include <QBuffer>

#include "raportti/raportinlaatija.h"

#include "db/tositetyyppimodel.h"

#include "tilinpaatoseditori/tilinpaatostulostaja.h"
#include "naytin/liitetulostaja.h"

AineistoDialog::AineistoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AineistoDialog),
    pdfDoc_{new QPdfDocument(this)},
    puskuri_{new QBuffer(this)}


{
    ui->setupUi(this);
    ui->tilikausiCombo->setModel(kp()->tilikaudet());

    lataaDialogi();

    connect( ui->tilikausiCombo, &QComboBox::currentTextChanged, this, &AineistoDialog::paivitaNimi);
    connect( ui->tiedostoNappi, &QPushButton::clicked, this, &AineistoDialog::vaihdaNimi);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] {kp()->ohje("tilikaudet/aineisto/");});

    connect( pdfDoc_, &QPdfDocument::statusChanged, this, &AineistoDialog::pdfTilaVaihtuu);
}

AineistoDialog::~AineistoDialog()
{
    delete ui;
}

void AineistoDialog::aineisto(const QDate &pvm, const QString &kieli)
{
    ui->tilikausiCombo->setCurrentIndex(kp()->tilikaudet()->indeksiPaivalle(pvm));
    paivitaNimi();
    if(!kieli.isEmpty())
        ui->kieliCombo->setCurrentIndex(ui->kieliCombo->findData(kieli));

    show();
}

void AineistoDialog::accept()
{
    QDialog::accept();

    tallennaDialogi();

    progress = new QProgressDialog(tr("Muodostetaan aineistoa. Tämä voi kestää useamman minuutin."), tr("Peruuta"), 0, 200);
    progress->setMinimumDuration(10);
    tilauskesken_ = true;


    kieli_ = ui->kieliCombo->currentData().toString();
    tilikausi_ = kp()->tilikaudet()->tilikausiIndeksilla(ui->tilikausiCombo->currentIndex());

    QPdfWriter *writer = new QPdfWriter(ui->tiedostoEdit->text());
    writer->setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    writer->setTitle(tulkkaa("Kirjanpitoaineisto %1", kieli_).arg(tilikausi_.kausivaliTekstina()));
    writer->setCreator(QString("%1 %2").arg(qApp->applicationName(), qApp->applicationVersion()));
    writer->setPageSize( QPageSize( QPageSize::A4));
    writer->setResolution( ui->dpiSlider->value() );

    writer->setPageMargins( QMarginsF(20,10,10,10), QPageLayout::Millimeter );

    painter = new QPainter( writer );
    device = writer;

    painter->setFont(QFont("FreeSans",8));
    rivinkorkeus_ = painter->fontMetrics().height();

    tilaaRaportit();
    jatkaTositelistaan();

}

void AineistoDialog::lataaDialogi()
{
    ui->raportitGroup->setChecked(kp()->settings()->value("AineistoRaportit", true).toBool());
    ui->taseCheck->setChecked(kp()->settings()->value("AineistoTase", true).toBool());
    ui->tuloslaskelmaCheck->setChecked(kp()->settings()->value("AineistoTulos", true).toBool());
    ui->erittelyCheck->setChecked(kp()->settings()->value("AineistoErittely", true).toBool());
    ui->paivakirjaCheck->setChecked(kp()->settings()->value("AineistoPaivakirja", true).toBool());
    ui->paakirjaCheck->setChecked(kp()->settings()->value("AineistoPaakirja", true).toBool());
    ui->myyntilaskutCheck->setChecked(kp()->settings()->value("AineistoMyynti", true).toBool());
    ui->ostolaskutCheck->setChecked(kp()->settings()->value("AineistoOstot", true).toBool());
    ui->tositeluetteloCheck->setChecked(kp()->settings()->value("AineistoLuettelo", true).toBool());

    int tluettelo = kp()->settings()->value("AineistoTililuetteloTaso", 2).toInt();
    ui->tililuetteloGroup->setChecked(kp()->settings()->value("AineistoTililuettelo", true).toBool());
    ui->kaikkitlRadio->setChecked(tluettelo == 3);
    ui->laajuustlRadio->setChecked(tluettelo == 2);
    ui->kirjauksettlRadio->setChecked(tluettelo == 1);

    ui->tiedotGrop->setChecked(kp()->settings()->value("AineistoTiedot", true).toBool());
    ui->muistiinpanotCheck->setChecked(kp()->settings()->value("AineistoMuistiinpanot", true).toBool());
    ui->tilioinnitCheck->setChecked(kp()->settings()->value("AineistoTilioinnit", true).toBool());
    ui->kaikkiCheck->setChecked(kp()->settings()->value("AineistoKaikkitositteet", true).toBool());
    ui->liitteetGroup->setChecked(kp()->settings()->value("AineistoLiitteet", true).toBool());
}

void AineistoDialog::tallennaDialogi()
{
    kp()->settings()->setValue("AineistoRaportit", ui->raportitGroup->isChecked() );
    kp()->settings()->setValue("AineistoTase", ui->taseCheck->isChecked() );
    kp()->settings()->setValue("AineistoTulos", ui->tuloslaskelmaCheck->isChecked());
    kp()->settings()->setValue("AineistoErittely", ui->erittelyCheck->isChecked());
    kp()->settings()->setValue("AineistoPaivakirja", ui->paivakirjaCheck->isChecked());
    kp()->settings()->setValue("AineistoPaakirja", ui->paakirjaCheck->isChecked());
    kp()->settings()->setValue("AineistoMyynti", ui->myyntilaskutCheck->isChecked());
    kp()->settings()->setValue("AineistoOstot", ui->ostolaskutCheck->isChecked());
    kp()->settings()->setValue("AineistoLuettelo", ui->tositeluetteloCheck->isChecked());

    kp()->settings()->setValue("AineistoTililuetteloTaso", ui->kaikkitlRadio->isChecked() ? 3 : ( ui->kirjauksettlRadio->isChecked() ? 1 : 2) );
    kp()->settings()->setValue("AineistoTililuettelo", ui->tililuetteloGroup->isChecked());
    kp()->settings()->setValue("AineistoTiedot", ui->tiedotGrop->isChecked());
    kp()->settings()->setValue("AineistoMuistiinpanot", ui->muistiinpanotCheck->isChecked());
    kp()->settings()->setValue("AineistoTilioinnit", ui->tilioinnitCheck->isChecked());
    kp()->settings()->setValue("AineistoKaikkitositteet", ui->kaikkiCheck->isChecked());
    kp()->settings()->setValue("AineistoLiitteet", ui->liitteetGroup->isChecked());

}

void AineistoDialog::paivitaNimi()
{
    if(!nimivaihdettu_) {
        QDir hakemisto;
        QString arkistopolku = kp()->settings()->value("arkistopolku/" + kp()->asetukset()->asetus(AsetusModel::UID)).toString();
        if( !arkistopolku.isEmpty() && QFile::exists(arkistopolku))
            hakemisto.cd(arkistopolku);
        QString nimi = kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi);
        nimi.replace(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption),"");
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla(ui->tilikausiCombo->currentIndex());
        ui->tiedostoEdit->setText(hakemisto.absoluteFilePath(QString("%1-%2.pdf").arg(nimi, kausi.pitkakausitunnus())));
    }
}

void AineistoDialog::vaihdaNimi()
{
    QString nimi = QFileDialog::getSaveFileName(this, tr("Tallenna aineisto tiedostoon"),
                                               ui->tiedostoEdit->text(),  tr("Pdf-tiedosto (*.pdf)"));
    if( !nimi.isEmpty())
        ui->tiedostoEdit->setText(nimi);
}

void AineistoDialog::tilaaRaportit()
{
    kirjoittajat_.resize(10);

    if( ui->raportitGroup->isChecked()) {

        if( ui->taseCheck->isChecked()) {
            RaporttiValinnat tase = raportti("tase/yleinen");
            tilaaRaportti(tase);
        }

        if( ui->tuloslaskelmaCheck->isChecked()) {
            RaporttiValinnat tulos = raportti("tulos/yleinen");
            tilaaRaportti(tulos);
        }

        if( ui->erittelyCheck->isChecked()) {
            RaporttiValinnat erittely = raportti("taseerittely");
            tilaaRaportti(erittely);
        }

        if( ui->paivakirjaCheck->isChecked()) {
            RaporttiValinnat paivakirja = raportti("paivakirja");
            tilaaRaportti(paivakirja);
        }

        if( ui->paakirjaCheck->isChecked()) {
            RaporttiValinnat paakirja = raportti("paakirja");
            tilaaRaportti(paakirja);

        }
    }

    if( ui->tililuetteloGroup->isChecked()) {
        RaporttiValinnat tililuettelo = raportti("tililuettelo");
        if( ui->kaikkitlRadio->isChecked())
            tililuettelo.aseta(RaporttiValinnat::LuettelonTilit,"kaikki");
        else if(ui->laajuustlRadio->isChecked())
            tililuettelo.aseta(RaporttiValinnat::LuettelonTilit,"kaytossa");
        else
            tililuettelo.aseta(RaporttiValinnat::LuettelonTilit, "kirjatut");
        tilaaRaportti(tililuettelo);
    }

    if( ui->raportitGroup->isChecked()) {

        if( ui->myyntilaskutCheck->isChecked()) {
            RaporttiValinnat myyntiLaskuLuettelo = raportti("laskut");
            myyntiLaskuLuettelo.aseta(RaporttiValinnat::TiedostonNimi, "myyntilaskut.html");
            myyntiLaskuLuettelo.aseta(RaporttiValinnat::LaskuTyyppi, "myynti");
            myyntiLaskuLuettelo.aseta(RaporttiValinnat::VainAvoimet);
            myyntiLaskuLuettelo.aseta(RaporttiValinnat::AlkuPvm, kp()->tilikaudet()->kirjanpitoAlkaa());
            tilaaRaportti(myyntiLaskuLuettelo);
        }

        if( ui->ostolaskutCheck->isChecked()) {
            RaporttiValinnat ostoLaskuLuettelo = raportti("laskut");
            ostoLaskuLuettelo.aseta(RaporttiValinnat::TiedostonNimi, "ostolaskut.html");
            ostoLaskuLuettelo.aseta(RaporttiValinnat::LaskuTyyppi, "osto");
            ostoLaskuLuettelo.aseta(RaporttiValinnat::VainAvoimet);
            ostoLaskuLuettelo.aseta(RaporttiValinnat::AlkuPvm, kp()->tilikaudet()->kirjanpitoAlkaa());
            tilaaRaportti(ostoLaskuLuettelo);
        }


        if( ui->tositeluetteloCheck->isChecked()) {
            RaporttiValinnat tositeLuettelo = raportti("tositeluettelo");
            tositeLuettelo.aseta(RaporttiValinnat::VientiJarjestys, "tosite");
            tilaaRaportti(tositeLuettelo);
        }
    }

    progress->setMaximum(tilattuja_ * 30);
    tilauskesken_ = false;


}



void AineistoDialog::jatkaTositelistaan()
{
    if( tilauskesken_ || valmiita_ < tilattuja_)
        return;

    KpKysely *kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("alkupvm", tilikausi_.alkaa());
    kysely->lisaaAttribuutti("loppupvm", tilikausi_.paattyy());
    kysely->lisaaAttribuutti("jarjestys","tosite");
    connect( kysely, &KpKysely::vastaus, this, &AineistoDialog::tositeListaSaapuu);
    kysely->kysy();
}

void AineistoDialog::tulostaRaportit()
{
    TilinpaatosTulostaja::tulostaKansilehti( painter, tulkkaa("Kirjanpitoaineisto", kieli_), tilikausi_, kieli_);
    sivu_ = 2;

    kirjoittajat_.resize(tilattuja_);
    for( auto& rk : kirjoittajat_) {
        if( !rk.riveja() )
            continue;
        device->newPage();
        sivu_ += rk.tulosta(device, painter, false, sivu_);
        progress->setValue(progress->value() + 5);
        qApp->processEvents();
    }
}

void AineistoDialog::tositeListaSaapuu(QVariant *data)
{
    tositteet_ = data->toList();
    progress->setMaximum( tilattuja_ * 10 + tositteet_.count() + 1);
    tositepnt_ = 0;

    tulostaRaportit();
    painter->translate(0, painter->window().height() - painter->transform().dy());

    if( kp()->onkoPilvessa()) {
        tilaaSeuraavaTosite();
    } else {
        // Paikallisessa toteutuksessa puretaan välillä pinoa
        while( tositepnt_ < tositteet_.count()) {
            tilaaSeuraavaTosite();
        }
        valmis();
    }
}

void AineistoDialog::tilaaSeuraavaTosite()
{
    if( tositepnt_ == tositteet_.count()) {
        valmis();
    } else {
        int tositeId = tositteet_.value(tositepnt_).toMap().value("id").toInt();
        tositepnt_++;

        KpKysely *tositeHaku = kpk(QString("/tositteet/%1").arg(tositeId));
        connect(tositeHaku, &KpKysely::vastaus, this, &AineistoDialog::tositeSaapuu);
        tositeHaku->kysy();
    }
}

void AineistoDialog::tositeSaapuu(QVariant *data)
{
    qApp->processEvents();
    if( progress && progress->wasCanceled()) {
        progress->close();
        delete painter;
        delete progress;
        progress = nullptr;
        return;
    }
    if(progress) {
        progress->setValue(progress->value() + 1);
    }

    nykyTosite_ = data->toMap();

    if( ui->liitteetGroup->isChecked()) {

        QVariantList liitelista = nykyTosite_.value("liitteet").toList();
        for(auto &liite : liitelista) {
            QVariantMap liiteMap = liite.toMap();
            QString tyyppi = liiteMap.value("tyyppi").toString();
            if( tyyppi == "application/pdf" || tyyppi == "image/jpeg") {
                liiteJono_.enqueue(qMakePair(liiteMap.value("id").toInt(), tyyppi));
            }
        }

    }

    // Liitteet, muistiinpanot, täydet, kaikkien tiedot
    // Selvitetään, onko liitteitä ja onko muistiinpanoja
    // Mahtuvatko liitteet, muistiinpanot ja viennit jne.

    int tyyppi = nykyTosite_.value("tyyppi").toInt();
    // Laskuissa muistiinpanot ovat tulostettuina joten niitä ei tarvitse toistaa aineistoissa
    QString muistiinpanot =  tyyppi < TositeTyyppi::MYYNTILASKU || tyyppi > TositeTyyppi::MAKSUMUISTUTUS ?  nykyTosite_.value("info").toString() : QString();
    int vienteja = nykyTosite_.value("viennit").toList().count();
    QRectF infoRectPuolikkaalla = painter->boundingRect(0,0, painter->window().width() / 2, painter->window().height(),Qt::TextWordWrap, muistiinpanot);

    bool tulostaErillinen = ui->tiedotGrop->isChecked() && (
                            ( ui->kaikkiCheck->isChecked() && liiteJono_.isEmpty() ) ||
                            ( ui->tilioinnitCheck->isChecked() && vienteja > 6) ||
                            ( ui->muistiinpanotCheck->isChecked() && (infoRectPuolikkaalla.height() > rivinkorkeus_ * 7 || liiteJono_.isEmpty()) && !muistiinpanot.isEmpty()) );


    tulostaAlatunniste_ = !tulostaErillinen && ui->tiedotGrop->isChecked();

    if( tulostaErillinen ) {
        // Tulostetaan tiliöinti- ja muistiinpano-osio erikseen
        const int sivua = LiiteTulostaja::tulostaTiedot(device, painter, nykyTosite_,
                                               sivu_-1, kieli_,
                                               ui->muistiinpanotCheck->isChecked() && !muistiinpanot.isEmpty(),
                                               ui->tilioinnitCheck->isChecked());
        sivu_  += sivua;
    }

    if( liiteJono_.isEmpty()) {
        if( kp()->onkoPilvessa()) {
            tilaaSeuraavaTosite();
        } else {
            // Paikallisella välitetään pinon kasvattamista
            return;
        }
    } else {
        tilaaLiite();
    }

}

void AineistoDialog::tilaaLiite()
{
    if( liiteJono_.isEmpty()) {
        if( kp()->onkoPilvessa())
            tilaaSeuraavaTosite();
        else
            return;
    } else {
        QPair<int,QString> liitetieto = liiteJono_.dequeue();
        KpKysely *liiteHaku = kpk(QString("/liitteet/%1").arg(liitetieto.first));
        connect( liiteHaku, &KpKysely::vastaus, this,
                 [this, liitetieto] (QVariant* data) { this->tilattuLiiteSaapuu(data, liitetieto.second); });
        liiteHaku->kysy();
    }
}

void AineistoDialog::tilattuLiiteSaapuu(QVariant *data, const QString &tyyppi)
{
    if( tyyppi == "application/pdf") {
        puskuri_->close();
        bytes_ = data->toByteArray();
        puskuri_->setBuffer(&bytes_);
        puskuri_->open(QIODevice::ReadOnly);
        pdfDoc_->load(puskuri_);
    } else if( tyyppi.startsWith("image")) {
        QByteArray ba = data->toByteArray();
        int sivua = LiiteTulostaja::tulostaKuvaLiite(
                    device, painter, ba, nykyTosite_, tulostaAlatunniste_, sivu_-1, kieli_ );


        if( sivua < 0)
            virhe_ = true;
        else {
            sivu_ += sivua;
            tulostaAlatunniste_ = false;
        }
        tilaaLiite();
    } else {
        tilaaLiite();
    }
}

void AineistoDialog::pdfTilaVaihtuu(QPdfDocument::Status status)
{
    if( status == QPdfDocument::Status::Ready) {
        painter->setFont(QFont("FreeSans",8));
        device->newPage();

        int rivinKorkeus = painter->fontMetrics().height();

        int pageCount = pdfDoc_->pageCount();
        for(int i=0; i < pageCount; i++)
        {
            painter->resetTransform();
            try {
                QSizeF koko = pdfDoc_->pagePointSize(i);
                QSizeF kohde( painter->window().width(), painter->window().height() - 12 * rivinKorkeus);
                koko.scale(kohde, Qt::KeepAspectRatio);

                QPdfDocumentRenderOptions options;
                if( koko.width() > koko.height())
                    options.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise270);

                QImage image = pdfDoc_->render(i, kohde.toSize(), options);
                painter->drawImage(0, rivinKorkeus*2, image);


                LiiteTulostaja::tulostaYlatunniste(painter, nykyTosite_, sivu_ , kieli_);
                painter->translate(0, painter->window().height() - ( i ? 1 : 8 ) * rivinKorkeus);
                sivu_++;

            }
                catch (std::bad_alloc&) {
                virhe_ = true;
            }

            if(tulostaAlatunniste_) {
                LiiteTulostaja::tulostaAlatunniste(painter, nykyTosite_, kieli_);
                tulostaAlatunniste_ = false;
            }

            if( i < pageCount - 1 )
                device->newPage();
        }
        painter->translate(0, painter->window().height() - painter->transform().dy());
        tilaaLiite();

    } else if( status == QPdfDocument::Status::Error) {
        virhe_ = true;
        tilaaLiite();
    }
}




void AineistoDialog::valmis()
{
    if(!painter)
        return;

    painter->end();
    if( virhe_) {
        QMessageBox::critical(nullptr, tr("Virhe aineiston muodostamisessa"),
                              tr("Tositteiden muodostamisessa aineistoksi tapahtui virhe.\n\n"
                                 "Todennäköisesti liitetiedostojen koko yhteensä on liian suuri, jotta ohjelma pystyisi muodostamaan niistä kaikista "
                                 "yhden pdf-tiedoston.\n\n"
                                 "Voit kuitenkin käyttää Arkisto-toimintoa muodostaaksesi kirjanpidostasi arkiston."));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(ui->tiedostoEdit->text()));
    }
    if(progress) {
        progress->close();
        delete progress;
        progress = nullptr;
    }
    delete painter;
    painter = nullptr;

}

RaporttiValinnat AineistoDialog::raportti(const QString &tyyppi) const
{
    RaporttiValinnat valinnat(tyyppi);
    valinnat.aseta(RaporttiValinnat::Kieli, kieli_ );
    valinnat.aseta(RaporttiValinnat::AlkuPvm, tilikausi_.alkaa());
    valinnat.aseta(RaporttiValinnat::LoppuPvm, tilikausi_.paattyy());
    valinnat.aseta(RaporttiValinnat::SaldoPvm, tilikausi_.paattyy());
    valinnat.aseta(RaporttiValinnat::LuetteloPvm, tilikausi_.paattyy());
    valinnat.aseta(RaporttiValinnat::TulostaKumppani);
    valinnat.aseta(RaporttiValinnat::TulostaSummarivit);
    valinnat.aseta(RaporttiValinnat::TulostaErittely);
    if( kp()->kohdennukset()->kohdennuksia())
        valinnat.aseta(RaporttiValinnat::TulostaKohdennus);

    valinnat.lisaaSarake(RaporttiValintaSarake(tilikausi_.alkaa(), tilikausi_.paattyy()));
    Tilikausi edellinen = kp()->tilikaudet()->tilikausiPaivalle( tilikausi_.alkaa().addDays(-1) );
    if( edellinen.alkaa().isValid()) {
        valinnat.lisaaSarake(RaporttiValintaSarake(edellinen.alkaa(), edellinen.paattyy()));
    }

    return valinnat;
}

void AineistoDialog::tilaaRaportti(RaporttiValinnat &valinnat)
{
    valinnat.aseta(RaporttiValinnat::TilausJarjestysNumero, tilattuja_);
    tilattuja_++;
    RaportinLaatija* laatija = new RaportinLaatija(this);
    connect( laatija, &RaportinLaatija::raporttiValmis, this, &AineistoDialog::raporttiSaapuu);
    laatija->laadi(valinnat);
}

void AineistoDialog::raporttiSaapuu(const RaportinKirjoittaja &kirjoittaja, const RaporttiValinnat &valinnat)
{
    kirjoittajat_[ valinnat.arvo(RaporttiValinnat::TilausJarjestysNumero).toInt() ]  = RaportinKirjoittaja(kirjoittaja);
    valmiita_++;
    progress->setValue(progress->value() + 5);
    jatkaTositelistaan();
}




