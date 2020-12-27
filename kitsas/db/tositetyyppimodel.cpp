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
#include "tositetyyppimodel.h"
#include "db/kirjanpito.h"

#include <QJsonDocument>

TositeTyyppiTietue::TositeTyyppiTietue(TositeTyyppi::Tyyppi uKoodi, const QString &uNimi, const QString &uKuvake, bool uLisattavissa) :
    koodi(uKoodi), nimi( uNimi ), kuvake( QIcon( uKuvake )), lisattavissa( uLisattavissa)
{

}



TositeTyyppiModel::TositeTyyppiModel(QObject *parent)
    : QAbstractListModel (parent)
{
    lisaa(TositeTyyppi::MENO, "Meno", "poista");
    lisaa(TositeTyyppi::TULO, "Tulo", "lisaa");

    lisaa(TositeTyyppi::KULULASKU, "Kululasku", "tekstisivu", true);
    lisaa(TositeTyyppi::SAAPUNUTVERKKOLASKU, "Saapunut verkkolasku","verkkolasku", true);

    lisaa(TositeTyyppi::MYYNTILASKU, "Myyntilasku", "lasku", false);
    lisaa(TositeTyyppi::HYVITYSLASKU, "Hyvityslasku", "lasku", false);
    lisaa(TositeTyyppi::MAKSUMUISTUTUS, "Maksumuistutus", "lasku", false);


    lisaa(TositeTyyppi::SIIRTO, "Siirto", "siirra");
    lisaa(TositeTyyppi::TILIOTE, "Tiliote", "tiliote");
    lisaa(TositeTyyppi::PALKKA, "Palkka", "yrittaja");
    lisaa(TositeTyyppi::MUISTIO, "Muistio","kommentti");
    lisaa(TositeTyyppi::LIITETIETO, "Liitetieto", "liite");

    lisaa(TositeTyyppi::TUONTI, "Tuonti", "tuotiedosto", true);
    lisaa(TositeTyyppi::MUU, "Muu", "tyhja");

    lisaa(TositeTyyppi::TILINAVAUS, "Tilinavaus", "rahaa", false);
    lisaa(TositeTyyppi::ALVLASKELMA, "Alv-laskelma", "verotilitys", false);
    lisaa(TositeTyyppi::POISTOLASKELMA, "Poistolaskelma", "kirjalaatikko", false);
    lisaa(TositeTyyppi::JAKSOTUS, "Jaksotus", "ratas", false);
    lisaa(TositeTyyppi::TULOVERO, "Tulovero", "verotilitys", false);
}

int TositeTyyppiModel::rowCount(const QModelIndex & /* parent */) const
{
    return tyypit_.count();
}

QVariant TositeTyyppiModel::data(const QModelIndex &index, int role) const
{
    TositeTyyppiTietue tietue = tyypit_.value(index.row());

    if( role == Qt::DisplayRole || role == NimiRooli)
        return tietue.nimi;
    else if( role == KoodiRooli )
        return tietue.koodi;
    else if( role == Qt::DecorationRole)
        return tietue.kuvake;
    else if( role == LisattavissaRooli)
        return tietue.lisattavissa ? "K" : "E" ;

    return QVariant();
}

QString TositeTyyppiModel::nimi(int koodi) const
{
    return map_.value(koodi).nimi;
}

QIcon TositeTyyppiModel::kuvake(int koodi) const
{
    return map_.value(koodi).kuvake;
}

bool TositeTyyppiModel::onkolisattavissa(int koodi) const
{
    return map_.value(koodi).lisattavissa;
}

QString TositeTyyppiModel::sarja(int koodi, bool kateinen) const
{
    QVariantMap sarjamap = QJsonDocument::fromJson( kp()->asetukset()->asetus("tositesarjat").toUtf8() ).toVariant().toMap();

    if( kateinen && kp()->asetukset()->onko(AsetusModel::KATEISSARJAAN))
        return sarjamap.value("K","K").toString();

    if( !kp()->asetukset()->onko(AsetusModel::ERISARJAAN))
        return QString();

    if( koodi >= 1000)
        return sarjamap.value("*","JT").toString();

    return sarjamap.value( QString::number(koodi), "X" ).toString();
}



void TositeTyyppiModel::lisaa(TositeTyyppi::Tyyppi koodi, const QString &nimi, const QString &kuvake, bool lisattavissa)
{
    TositeTyyppiTietue tietue(koodi, nimi, ":/pic/" + kuvake + ".png", lisattavissa);
    tyypit_.append( tietue );
    map_.insert(koodi, tietue);
}
