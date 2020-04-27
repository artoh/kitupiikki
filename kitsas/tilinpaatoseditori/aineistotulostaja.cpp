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

AineistoTulostaja::AineistoTulostaja(QObject *parent) : QObject(parent)
{

}

AineistoTulostaja::~AineistoTulostaja()
{
    liitedatat_.clear();
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
    writer->setTitle(tr("Kirjanpitoaineisto %1").arg(kausi.kausivaliTekstina()));
    writer->setCreator(tr("Kitsas %1").arg(qApp->applicationVersion()));
    writer->setPageSize( QPdfWriter::A4);

    writer->setPageMargins( QMarginsF(25,10,10,10), QPageLayout::Millimeter );

    painter = new QPainter( writer );
    device = writer;

    tilaaRaportit();
}

void AineistoTulostaja::tulostaRaportit()
{
    progress = new QProgressDialog(tr("Muodostetaan aineistoa. Tämä voi kestää useamman minuutin."), tr("Peruuta"), 0, kirjoittajat_.count() + liitteet_.count());
    progress->setMinimumDuration(150);

    TilinpaatosTulostaja::tulostaKansilehti( painter, "Kirjanpitoaineisto", tilikausi_);

    int sivu = 2;

    for( auto rk : kirjoittajat_) {
        device->newPage();
        sivu += rk.tulosta(device, painter, false, sivu);
        qApp->processEvents();
        progress->setValue(progress->value() + 1);
        if( progress->wasCanceled()) {
            progress->close();
            delete painter;
            delete progress;
            return;
        }
    }
    liitepnt_ = 0;
    tilaaSeuraavaLiite();
}


void AineistoTulostaja::tilaaRaportit()
{
    tilattuja_ = 7;
    kirjoittajat_.resize(7);
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

    TositeLuettelo* luettelo = new TositeLuettelo(this);
    connect( luettelo, &TositeLuettelo::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(6, rk);});
    luettelo->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy(),
                       TositeLuettelo::TositeJarjestyksessa | TositeLuettelo::SamaTilikausi |
                       TositeLuettelo::TulostaSummat | TositeLuettelo::AsiakasToimittaja);
}

void AineistoTulostaja::tilaaLiitteet()
{
    KpKysely *kysely = kpk("/liitteet");
    kysely->lisaaAttribuutti("alkupvm", tilikausi_.alkaa());
    kysely->lisaaAttribuutti("loppupvm", tilikausi_.paattyy());
    connect( kysely, &KpKysely::vastaus, this, &AineistoTulostaja::liiteListaSaapuu);
    kysely->kysy();
}


void AineistoTulostaja::raporttiSaapuu(int raportti, RaportinKirjoittaja rk)
{
    kirjoittajat_[raportti] = rk;
    tilattuja_--;
    if( !tilattuja_)
        tilaaLiitteet();
}

void AineistoTulostaja::liiteListaSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    liitteet_ = lista;
    tulostaRaportit();
}

void AineistoTulostaja::tilaaSeuraavaLiite()
{
    if( liitepnt_ == liitteet_.count()) {
        painter->end();
        QDesktopServices::openUrl(QUrl::fromLocalFile(polku_));
        progress->close();
        delete progress;
        delete painter;
        return;
    }

    QVariantMap map = liitteet_.value(liitepnt_).toMap();
    liitepnt_++;

    QString tyyppi = map.value("tyyppi").toString();
    if( tyyppi != "application/pdf" &&
        !tyyppi.startsWith("image/"))
    {
        tilaaSeuraavaLiite();
        return;
    }

    int liiteid = map.value("id").toInt();
    KpKysely *liitehaku = kpk(QString("/liitteet/%1").arg(liiteid));
    connect( liitehaku, &KpKysely::vastaus,
             [this, map] (QVariant* data) { this->tulostaLiite(data, map); });
    liitehaku->kysy();
}

void AineistoTulostaja::tulostaLiite(QVariant *data, const QVariantMap &map)
{
    qApp->processEvents();
    if( progress->wasCanceled()) {
        progress->close();
        delete painter;
        delete progress;
        return;
    }

    QByteArray liite = data->toByteArray();
    device->newPage();
    LiiteTulostaja::tulostaLiite(device, painter, liite, map.value("tyyppi").toString(),
                                         map.value("pvm").toDate(), map.value("sarja").toString(), map.value("tunniste").toInt());
    progress->setValue(progress->value() + 1);
    tilaaSeuraavaLiite();
}
