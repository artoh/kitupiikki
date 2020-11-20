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


#include "arkistoija/arkistohakemistodialogi.h"

#include "aineistotulostaja.h"

#include "db/kirjanpito.h"

#include "raportti/raportoija.h"
#include "raportti/paakirja.h"
#include "raportti/paivakirja.h"
#include "raportti/taseerittelija.h"
#include "raportti/tilikarttalistaaja.h"
#include "raportti/tositeluettelo.h"
#include "raportti/laskuraportteri.h"

#include "tilinpaatostulostaja.h"
#include "naytin/liitetulostaja.h"
#include "naytin/naytinikkuna.h"
#include "naytin/naytinview.h"

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

AineistoTulostaja::AineistoTulostaja(QObject *parent) : QObject(parent)
{

}

AineistoTulostaja::~AineistoTulostaja()
{
    qDebug() << "~Aineisto";
}

void AineistoTulostaja::tallennaAineisto(Tilikausi kausi, const QString &kieli)
{
    tilikausi_ = kausi;
    kieli_ = kieli;

    QString arkistopolku = kp()->settings()->value("arkistopolku/" + kp()->asetus("UID")).toString();
    if( arkistopolku.isEmpty() || !QFile::exists(arkistopolku))
        arkistopolku = ArkistohakemistoDialogi::valitseArkistoHakemisto();
    if( arkistopolku.isEmpty())
        return;

    QDir hakemisto(arkistopolku );
    QString nimi = kp()->asetukset()->asetus("Nimi");
    nimi.replace(QRegularExpression("\\W",QRegularExpression::UseUnicodePropertiesOption),"");
    polku_ = hakemisto.absoluteFilePath(QString("%1-%2.pdf").arg(nimi).arg(kausi.pitkakausitunnus()));

    QPdfWriter *writer = new QPdfWriter(polku_);
    writer->setTitle(tulkkaa("Kirjanpitoaineisto %1", kieli_).arg(kausi.kausivaliTekstina()));
    writer->setCreator(tr("Kitsas %1").arg(qApp->applicationVersion()));
    writer->setPageSize( QPdfWriter::A4);

    writer->setPageMargins( QMarginsF(20,10,10,10), QPageLayout::Millimeter );

    painter = new QPainter( writer );
    device = writer;

    tilaaRaportit();
}

void AineistoTulostaja::tulostaRaportit()
{
    TilinpaatosTulostaja::tulostaKansilehti( painter, tulkkaa("Kirjanpitoaineisto", kieli_), tilikausi_);

    sivu_ = 2;

    for( auto rk : kirjoittajat_) {
        device->newPage();
        sivu_ += rk.tulosta(device, painter, false, sivu_);
        qApp->processEvents();
        progress->setValue(progress->value() + 1);
        if( progress->wasCanceled()) {
            progress->close();
            delete painter;
            delete progress;
            return;
        }
    }

    sivu_--;
}

void AineistoTulostaja::tilaaTositeLista()
{
    KpKysely *kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("alkupvm", tilikausi_.alkaa());
    kysely->lisaaAttribuutti("loppupvm", tilikausi_.paattyy());
    kysely->lisaaAttribuutti("jarjestys","tosite");
    connect( kysely, &KpKysely::vastaus, this, &AineistoTulostaja::tositeListaSaapuu);
    kysely->kysy();
}

void AineistoTulostaja::tositeListaSaapuu(QVariant *data)
{
    tositteet_ = data->toList();

    progress = new QProgressDialog(tr("Muodostetaan aineistoa. Tämä voi kestää useamman minuutin."), tr("Peruuta"), 0, kirjoittajat_.count() + tositteet_.count());
    progress->setMinimumDuration(150);

    tulostaRaportit();

    tositepnt_ = 0;
    tilaaSeuraavaTosite();

}


void AineistoTulostaja::tilaaRaportit()
{
    tilattuja_ = 9;
    kirjoittajat_.resize(tilattuja_);
    QString kieli = "fi";


    Raportoija* tase = new Raportoija("tase/yleinen", kieli, this, Raportoija::TASE);
    tase->lisaaTasepaiva(tilikausi_.paattyy());
    connect(tase, &Raportoija::valmis, [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(0, rk);});
    tase->kirjoita(true);

    Raportoija* tulos = new Raportoija("tulos/yleinen", kieli, this, Raportoija::TULOSLASKELMA);
    tulos->lisaaKausi(tilikausi_.alkaa(), tilikausi_.paattyy());
    connect(tulos, &Raportoija::valmis, [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(1, rk);});
    tulos->kirjoita(true);

    TaseErittelija *erittely = new TaseErittelija(this);
    connect( erittely, &TaseErittelija::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(2, rk);});
    erittely->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy());

    Paivakirja *paivakirja = new Paivakirja(this);
    connect( paivakirja, &Paivakirja::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(3, rk);});
    paivakirja->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy(), Paivakirja::AsiakasToimittaja + Paivakirja::TulostaSummat +  (kp()->kohdennukset()->kohdennuksia() ? Paivakirja::TulostaKohdennukset : 0));

    Paakirja *paakirja = new Paakirja(this);
    connect( paakirja, &Paakirja::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(4, rk);});
    paakirja->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy(), Paakirja::AsiakasToimittaja +  Paakirja::TulostaSummat + (kp()->kohdennukset()->kohdennuksia() ? Paivakirja::TulostaKohdennukset : 0));

    TiliKarttaListaaja* tililuettelo = new TiliKarttaListaaja(this);
    connect( tililuettelo, &TiliKarttaListaaja::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(5, rk);});
    tililuettelo->kirjoita(TiliKarttaListaaja::KAYTOSSA_TILIT, tilikausi_, true, false, tilikausi_.paattyy(), false);

    LaskuRaportteri* myyntilaskut = new LaskuRaportteri(this);
    connect( myyntilaskut, &LaskuRaportteri::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(6, rk);});
    myyntilaskut->kirjoita( LaskuRaportteri::TulostaSummat | LaskuRaportteri::Myyntilaskut | LaskuRaportteri::VainAvoimet, tilikausi_.paattyy());

    LaskuRaportteri* ostolaskut = new LaskuRaportteri(this);
    connect( ostolaskut, &LaskuRaportteri::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(7,rk);});
    ostolaskut->kirjoita( LaskuRaportteri::TulostaSummat | LaskuRaportteri::Ostolaskut | LaskuRaportteri::VainAvoimet, tilikausi_.paattyy());


    TositeLuettelo* luettelo = new TositeLuettelo(this);
    connect( luettelo, &TositeLuettelo::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(8, rk);});
    luettelo->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy(),
                       TositeLuettelo::TositeJarjestyksessa | TositeLuettelo::SamaTilikausi |
                       TositeLuettelo::AsiakasToimittaja);
}

void AineistoTulostaja::raporttiSaapuu(int raportti, RaportinKirjoittaja rk)
{
    kirjoittajat_[raportti] = rk;
    tilattuja_--;
    if( !tilattuja_)
        tilaaTositeLista();
}

void AineistoTulostaja::tilaaSeuraavaTosite()
{
    if( tositepnt_ == tositteet_.count()) {
        valmis();
    } else {
        int tositeId = tositteet_.value(tositepnt_).toMap().value("id").toInt();
        tositepnt_++;

        progress->setValue(progress->value() + 1);

        KpKysely *tositeHaku = kpk(QString("/tositteet/%1").arg(tositeId));
        connect(tositeHaku, &KpKysely::vastaus, this, &AineistoTulostaja::tositeSaapuu);
        tositeHaku->kysy();
    }
}

void AineistoTulostaja::tositeSaapuu(QVariant *data)
{
    qApp->processEvents();
    if( progress->wasCanceled()) {
        progress->close();
        delete painter;
        delete progress;
        return;
    }

    nykyTosite_ = data->toMap();

    QVariantList liitelista = nykyTosite_.value("liitteet").toList();
    for(auto liite : liitelista) {
        QVariantMap liiteMap = liite.toMap();
        QString tyyppi = liiteMap.value("tyyppi").toString();
        if( tyyppi == "application/pdf" || tyyppi == "image/jpeg") {
            liiteJono_.enqueue(qMakePair(liiteMap.value("id").toInt(), tyyppi));
        }
    }

    ensisivu_ = true;

    if(liiteJono_.isEmpty()) {
        if(!nykyTosite_.value("info").toString().isEmpty()) {
            if(painter->window().height() - painter->transform().dy() < LiiteTulostaja::muistiinpanojenKorkeus(painter, nykyTosite_))  {
                device->newPage();
                painter->resetTransform();
                sivu_++;
            } else {
                painter->drawLine(0,0,painter->window().width(),0);
                painter->translate(0, painter->fontMetrics().height());
            }
            LiiteTulostaja::tulostaMuistiinpanot(painter, nykyTosite_, sivu_, kieli_);

        }
        tilaaSeuraavaTosite();
    } else {
        tilaaLiite();
    }

}

void AineistoTulostaja::tilaaLiite()
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

void AineistoTulostaja::tilattuLiiteSaapuu(QVariant *data, const QString &tyyppi)
{

    QByteArray ba = data->toByteArray();
    device->newPage();
    int sivua = LiiteTulostaja::tulostaLiite(
                device, painter,
                ba,
                tyyppi,
                nykyTosite_,
                ensisivu_,
                sivu_,
                kieli_);
    if( sivua < 0)
        virhe_ = true;
    else
        sivu_ += sivua;

    ensisivu_ = false;
    tilaaLiite();
}

void AineistoTulostaja::valmis()
{
    painter->end();
    if( virhe_) {
        QMessageBox::critical(nullptr, tr("Virhe aineiston muodostamisessa"),
                              tr("Tositteiden muodostamisessa aineistoksi tapahtui virhe.\n\n"
                                 "Todennäköisesti liitetiedostojen koko yhteensä on liian suuri, jotta niistä ohjelma pystyisi muodostamaan niistä "
                                 "yhden suuren pdf-tiedoston.\n\n"
                                 "Voit kuitenkin käyttää Arkisto-toimintoa muodostaaksesi kirjanpidostasi arkiston."));
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(polku_));
    }
    progress->close();
    delete progress;
    delete painter;

    return;

}
