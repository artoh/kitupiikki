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



#include "aineistotulostaja.h"

#include "db/kirjanpito.h"

#include "raportti/paakirja.h"
#include "raportti/paivakirja.h"
#include "raportti/taseerittelija.h"
#include "raportti/tilikarttalistaaja.h"
#include "raportti/tositeluettelo.h"

#include "tilinpaatostulostaja.h"
#include "naytin/liitetulostaja.h"

#include <QPdfWriter>
#include <QPainter>
#include <QApplication>
#include <QDesktopServices>

AineistoTulostaja::AineistoTulostaja(QObject *parent) : QObject(parent)
{

}

void AineistoTulostaja::naytaAineisto(Tilikausi kausi, const QString &kieli, bool tallenna)
{
    tilikausi_ = kausi;
    kieli_ = kieli;
    tallenna_ = tallenna;
    tilaaRaportit();
}

void AineistoTulostaja::tulosta(QPagedPaintDevice *writer) const
{
    writer->setPageSize( QPdfWriter::A4);

    writer->setPageMargins( QMarginsF(25,10,10,10), QPageLayout::Millimeter );
    QPainter painter( writer );

    TilinpaatosTulostaja::tulostaKansilehti( &painter, "Kirjanpitoaineisto", tilikausi_);

    for( auto rk : kirjoittajat_) {
        writer->newPage();
        rk.tulosta(writer, &painter);
    }

    bool valiennen = true;

    // Vielä liitteet
    for( auto rivi : liitteet_) {
        QVariantMap map = rivi.toMap();
        int id = map.value("id").toInt();
        if(valiennen)
            writer->newPage();
        valiennen = LiiteTulostaja::tulostaLiite(writer, &painter, liitedatat_.value(id), map.value("tyyppi").toString(),
                                     map.value("pvm").toDate(), map.value("sarja").toString(), map.value("tunniste").toInt());
    }
    painter.end();
}

QString AineistoTulostaja::otsikko() const
{
    return tr("Kirjanpitoaineisto");
}

void AineistoTulostaja::tilaaRaportit()
{
    tilattuja_ = 5;
    kirjoittajat_.resize(5);

    TaseErittelija *erittely = new TaseErittelija(this);
    connect( erittely, &TaseErittelija::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(0, rk);});
    erittely->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy());

    Paivakirja *paivakirja = new Paivakirja(this);
    connect( paivakirja, &Paivakirja::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(1, rk);});
    paivakirja->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy());

    Paakirja *paakirja = new Paakirja(this);
    connect( paakirja, &Paakirja::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(2, rk);});
    paakirja->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy());

    TiliKarttaListaaja* tililuettelo = new TiliKarttaListaaja(this);
    connect( tililuettelo, &TiliKarttaListaaja::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(3, rk);});
    tililuettelo->kirjoita(TiliKarttaListaaja::KAYTOSSA_TILIT, tilikausi_, true, false, tilikausi_.paattyy(), false);

    TositeLuettelo* luettelo = new TositeLuettelo(this);
    connect( luettelo, &TositeLuettelo::valmis,
             [this] (RaportinKirjoittaja rk) { this->raporttiSaapuu(4, rk);});
    luettelo->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy(),
                       TositeLuettelo::TositeJarjestyksessa | TositeLuettelo::SamaTilikausi |
                       TositeLuettelo::TulostaSummat);
}

void AineistoTulostaja::tilaaLiitteet()
{
    KpKysely *kysely = kpk("/liitteet");
    kysely->lisaaAttribuutti("alkupvm", tilikausi_.alkaa());
    kysely->lisaaAttribuutti("loppupvm", tilikausi_.paattyy());
    connect( kysely, &KpKysely::vastaus, this, &AineistoTulostaja::liiteListaSaapuu);
    kysely->kysy();
}

void AineistoTulostaja::seuraavaLiite()
{
    if( liitepnt_ == liitteet_.count()) {
        if( tallenna_)
            tallennaAineisto();
        else
            esikatsele();
    } else {
        int liiteid = liitteet_.value(liitepnt_).toMap().value("id").toInt();

        KpKysely *liitehaku = kpk(QString("/liitteet/%1").arg(liiteid));
        connect( liitehaku, &KpKysely::vastaus,
                 [this, liiteid] (QVariant* data) { this->liiteSaapuu(liiteid, data); });
        liitepnt_++;
        liitehaku->kysy();
    }
}

void AineistoTulostaja::tallennaAineisto()
{
    QString polku = kp()->tilapainen( QString("aineisto-%1.pdf").arg(tilikausi_.pitkakausitunnus()) );
    QPdfWriter writer(polku);
    writer.setTitle(tr("Kirjanpitoaineisto %1").arg(tilikausi_.kausivaliTekstina()));
    writer.setCreator(QString("%1 %2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ));
    tulosta(&writer);
    QDesktopServices::openUrl(QUrl::fromLocalFile(polku));
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
    seuraavaLiite();
}

void AineistoTulostaja::liiteSaapuu(int liiteid, QVariant *var)
{
    liitedatat_.insert(liiteid, var->toByteArray());
    seuraavaLiite();
}
