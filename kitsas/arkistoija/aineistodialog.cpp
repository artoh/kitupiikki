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

#include "tools/tulkki.h"
#include "db/kirjanpito.h"
#include <QSettings>
#include <QFileDialog>

#include <QPdfWriter>
#include <QPainter>
#include <QApplication>
#include <QDesktopServices>
#include <QProgressDialog>
#include <QDebug>
#include <QSettings>
#include <QPdfWriter>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QMessageBox>

#include <QDir>

#include "raportti/raportoija.h"
#include "raportti/paakirja.h"
#include "raportti/paivakirja.h"
#include "raportti/taseerittelija.h"
#include "raportti/tilikarttalistaaja.h"
#include "raportti/tositeluettelo.h"
#include "raportti/laskuraportteri.h"

#include "tilinpaatoseditori/tilinpaatostulostaja.h"
#include "naytin/liitetulostaja.h"

AineistoDialog::AineistoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AineistoDialog)
{
    ui->setupUi(this);
    ui->tilikausiCombo->setModel(kp()->tilikaudet());

    Tulkki::alustaKieliCombo(ui->kieliCombo);

    connect( ui->tilikausiCombo, &QComboBox::currentTextChanged, this, &AineistoDialog::paivitaNimi);
    connect( ui->tiedostoNappi, &QPushButton::clicked, this, &AineistoDialog::vaihdaNimi);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] {kp()->ohje("tilikaudet/#kirjanpitoaineisto");});
}

AineistoDialog::~AineistoDialog()
{
    delete ui;
}

void AineistoDialog::aineisto(const QDate &pvm)
{
    ui->tilikausiCombo->setCurrentIndex(kp()->tilikaudet()->indeksiPaivalle(pvm));
    paivitaNimi();

    show();
}

void AineistoDialog::accept()
{
    QDialog::accept();

    progress = new QProgressDialog(tr("Muodostetaan aineistoa. Tämä voi kestää useamman minuutin."), tr("Peruuta"), 0, 200);
    progress->setMinimumDuration(10);


    kieli_ = ui->kieliCombo->currentData().toString();
    tilikausi_ = kp()->tilikaudet()->tilikausiIndeksilla(ui->tilikausiCombo->currentIndex());

    QPdfWriter *writer = new QPdfWriter(ui->tiedostoEdit->text());
    writer->setTitle(tulkkaa("Kirjanpitoaineisto %1", kieli_).arg(tilikausi_.kausivaliTekstina()));
    writer->setCreator(QString("%1 %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()));
    writer->setPageSize( QPdfWriter::A4);

    writer->setPageMargins( QMarginsF(20,10,10,10), QPageLayout::Millimeter );

    painter = new QPainter( writer );
    device = writer;

    painter->setFont(QFont("FreeSans",8));
    rivinkorkeus_ = painter->fontMetrics().height();

    tilaaRaportit();

}

void AineistoDialog::paivitaNimi()
{
    if(!nimivaihdettu_) {
        QDir hakemisto;
        QString arkistopolku = kp()->settings()->value("arkistopolku/" + kp()->asetus("UID")).toString();
        if( !arkistopolku.isEmpty() && QFile::exists(arkistopolku))
            hakemisto.cd(arkistopolku);
        QString nimi = kp()->asetukset()->asetus("Nimi");
        nimi.replace(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption),"");
        Tilikausi kausi = kp()->tilikaudet()->tilikausiIndeksilla(ui->tilikausiCombo->currentIndex());
        ui->tiedostoEdit->setText(hakemisto.absoluteFilePath(QString("%1-%2.pdf").arg(nimi).arg(kausi.pitkakausitunnus())));
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
    tilauskesken_ = true;
    int tilattuja = 0;

    if( ui->taseCheck->isChecked()) {
        Raportoija* tase = new Raportoija("tase/yleinen", kieli_, this, Raportoija::TASE);
        tase->lisaaTasepaiva(tilikausi_.paattyy());
        connect(tase, &Raportoija::valmis, [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        tase->kirjoita(true);
        tilattuja++;
    }

    if( ui->tuloslaskelmaCheck->isChecked()) {
        Raportoija* tulos = new Raportoija("tulos/yleinen", kieli_, this, Raportoija::TULOSLASKELMA);
        tulos->lisaaKausi(tilikausi_.alkaa(), tilikausi_.paattyy());
        connect(tulos, &Raportoija::valmis, [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        tulos->kirjoita(true);
        tilattuja++;
    }

    if( ui->erittelyCheck->isChecked()) {
        TaseErittelija *erittely = new TaseErittelija(this, kieli_);
        connect( erittely, &TaseErittelija::valmis,
                 [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        erittely->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy());
        tilattuja++;
    }

    if( ui->paivakirjaCheck->isChecked()) {
        Paivakirja *paivakirja = new Paivakirja(this, kieli_);
        connect( paivakirja, &Paivakirja::valmis,
                 [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        paivakirja->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy(), Paivakirja::AsiakasToimittaja + Paivakirja::TulostaSummat +  (kp()->kohdennukset()->kohdennuksia() ? Paivakirja::TulostaKohdennukset : 0));
        tilattuja++;
    }

    if( ui->paakirjaCheck->isChecked()) {
        Paakirja *paakirja = new Paakirja(this, kieli_);
        connect( paakirja, &Paakirja::valmis,
                 [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        paakirja->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy(), Paakirja::AsiakasToimittaja +  Paakirja::TulostaSummat + (kp()->kohdennukset()->kohdennuksia() ? Paivakirja::TulostaKohdennukset : 0));
        tilattuja++;
    }

    if( !ui->eitlRadio->isChecked()) {
        TiliKarttaListaaja* tililuettelo = new TiliKarttaListaaja(this);
        connect( tililuettelo, &TiliKarttaListaaja::valmis,
                 [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        tililuettelo->kirjoita( ui->kaikkitlRadio->isChecked() ? TiliKarttaListaaja::KAIKKI_TILIT :
                                                                 ( ui->laajuustlRadio->isChecked() ? TiliKarttaListaaja::KAYTOSSA_TILIT : TiliKarttaListaaja::KIRJATUT_TILIT),
                                tilikausi_, true, false, tilikausi_.paattyy(), false, kieli_);
        tilattuja++;
    }

    if( ui->myyntilaskutCheck->isChecked()) {
        LaskuRaportteri* myyntilaskut = new LaskuRaportteri(this, kieli_);
        connect( myyntilaskut, &LaskuRaportteri::valmis,
                 [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        myyntilaskut->kirjoita( LaskuRaportteri::TulostaSummat | LaskuRaportteri::Myyntilaskut | LaskuRaportteri::VainAvoimet, tilikausi_.paattyy());
        tilattuja++;
    }

    if( ui->ostolaskutCheck->isChecked()) {
        LaskuRaportteri* ostolaskut = new LaskuRaportteri(this, kieli_);
        connect( ostolaskut, &LaskuRaportteri::valmis,
                 [this, tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja,rk);});
        ostolaskut->kirjoita( LaskuRaportteri::TulostaSummat | LaskuRaportteri::Ostolaskut | LaskuRaportteri::VainAvoimet, tilikausi_.paattyy());
        tilattuja++;
    }


    if( ui->tositeluetteloCheck->isChecked()) {
        TositeLuettelo* luettelo = new TositeLuettelo(this, kieli_);
        connect( luettelo, &TositeLuettelo::valmis,
                 [this,tilattuja] (RaportinKirjoittaja rk) { this->raporttiSaapuu(tilattuja, rk);});
        luettelo->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy(),
                           TositeLuettelo::TositeJarjestyksessa | TositeLuettelo::SamaTilikausi |
                           TositeLuettelo::AsiakasToimittaja);
        tilattuja++;
    }
    tilattuja_ = tilattuja;
    progress->setMaximum(tilattuja_ * 30);
    tilauskesken_ = false;

    jatkaTositelistaan();
}

void AineistoDialog::raporttiSaapuu(int raportti, RaportinKirjoittaja rk)
{
    kirjoittajat_[raportti] = rk;
    valmiita_++;
    progress->setValue(progress->value() + 5);
    qDebug() << "Raportti saapuu " << progress->value() << " / " << progress->maximum();
    jatkaTositelistaan();
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
        device->newPage();
        sivu_ += rk.tulosta(device, painter, false, sivu_);
        progress->setValue(progress->value() + 5);
        qDebug() << "Raportti tulostetaan " << progress->value() << " / " << progress->maximum();
        qApp->processEvents();
    }
}

void AineistoDialog::tositeListaSaapuu(QVariant *data)
{
    tositteet_ = data->toList();
    progress->setMaximum( tilattuja_ * 10 + tositteet_.count() + 1);
    qDebug() << "Tositelista saapuu " << progress->value() << " / " << progress->maximum();
    tositepnt_ = 0;

    tulostaRaportit();
    tilaaSeuraavaTosite();
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
        qDebug() << "Tosite saapuu " << progress->value() << " / " << progress->maximum();
    }

    nykyTosite_ = data->toMap();

    QVariantList liitelista = nykyTosite_.value("liitteet").toList();
    for(auto &liite : liitelista) {
        QVariantMap liiteMap = liite.toMap();
        QString tyyppi = liiteMap.value("tyyppi").toString();
        if( tyyppi == "application/pdf" || tyyppi == "image/jpeg") {
            liiteJono_.enqueue(qMakePair(liiteMap.value("id").toInt(), tyyppi));
        }
    }

    // Liitteet, muistiinpanot, täydet, kaikkien tiedot
    // Selvitetään, onko liitteitä ja onko muistiinpanoja
    // Mahtuvatko liitteet, muistiinpanot ja viennit jne.

    QString muistiinpanot = nykyTosite_.value("info").toString();
    int vienteja = nykyTosite_.value("viennit").toList().count();
    QRectF infoRectPuolikkaalla = painter->boundingRect(0,0, painter->window().width() / 2, painter->window().height(),Qt::TextWordWrap, muistiinpanot);

    bool tulostaErillinen = ( ui->kaikkiCheck->isChecked() && liiteJono_.isEmpty() ) ||
                            ( ui->tilioinnitCheck->isChecked() && vienteja > 6) ||
                            ( ui->muistiinpanotCheck->isChecked() && (infoRectPuolikkaalla.height() > rivinkorkeus_ * 7 || liiteJono_.isEmpty()));


    tulostaAlatunniste_ = !tulostaErillinen;

    if( tulostaErillinen ) {
        // Tulostetaan tiliöinti- ja muistiinpano-osio erikseen
        // Tämä pitää vielä tehdä ;)
    }

    if( liiteJono_.isEmpty()) {
        tilaaSeuraavaTosite();
    } else {
        tilaaLiite();
    }

}

void AineistoDialog::tilaaLiite()
{
    if( liiteJono_.isEmpty()) {
        tilaaSeuraavaTosite();
    } else {
        QPair<int,QString> liitetieto = liiteJono_.dequeue();
        KpKysely *liiteHaku = kpk(QString("/liitteet/%1").arg(liitetieto.first));
        connect( liiteHaku, &KpKysely::vastaus,
                 [this, liitetieto] (QVariant* data) { this->tilattuLiiteSaapuu(data, liitetieto.second); });
        liiteHaku->kysy();
    }
}

void AineistoDialog::tilattuLiiteSaapuu(QVariant *data, const QString &tyyppi)
{
    QByteArray ba = data->toByteArray();
    device->newPage();
    int sivua = LiiteTulostaja::tulostaLiite(
                device, painter,
                ba,
                tyyppi,
                nykyTosite_,
                tulostaAlatunniste_,
                sivu_,
                kieli_);
    if( sivua < 0)
        virhe_ = true;
    else
        sivu_ += sivua;

    tulostaAlatunniste_ = false;
    tilaaLiite();
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



