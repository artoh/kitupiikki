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
#include "tosite.h"

#include "tositeviennit.h"
#include "db/kirjanpito.h"

#include <QJsonDocument>
#include <QDebug>

Tosite::Tosite(QObject *parent) :
    QObject(parent),
    viennit_(new TositeViennit(this))
{
    connect( viennit_, &TositeViennit::dataChanged, this, &Tosite::tarkasta );
    connect( viennit_, &TositeViennit::modelReset, this, &Tosite::tarkasta );
}

QVariant Tosite::data(int kentta) const
{
    return data_.value( avaimet__.at(kentta) );
}

void Tosite::setData(int kentta, QVariant arvo)
{
    if( arvo.toString().isEmpty() )
        data_.remove( avaimet__.at(kentta) );
    else
        data_.insert( avaimet__.at(kentta), arvo );
    tarkasta();
}

void Tosite::lataa(int tositeid)
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeid));
    connect(kysely, &KpKysely::vastaus, this, &Tosite::lataaData);
}

void Tosite::lataaData(QVariant *variant)
{
    data_ = variant->toMap();
    tallennettu_ = data_;

    viennit()->asetaViennit( data_.take("viennit").toList() );

    // toimittaja/asiakastiedot, liitteet ja loki

    emit ladattu();
}

void Tosite::tallenna()
{
    if( tallennuskesken_)
        return;

    tallennuskesken_ = true;
    KpKysely* kysely;
    if( data(ID).isNull())
        kysely = kpk( "/tositteet/", KpKysely::POST);
    else
        kysely = kpk( QString("/tositteet/%1").arg( data(ID).toInt() ), KpKysely::PUT);


    qDebug() << QJsonDocument::fromVariant(tallennettava() );

    connect(kysely, &KpKysely::vastaus, this, &Tosite::tallennusValmis  );
    connect(kysely, &KpKysely::virhe, this, &Tosite::tallennuksessaVirhe);

    kysely->kysy( tallennettava() );
}

void Tosite::tarkasta()
{
    bool muutettu = tallennettu_ != tallennettava();

    int virheet = 0;
    double debet = 0.0;
    double kredit = 0.0;

    // Tarkasta päivämäärät ja alvit

    for(int i=0; i < viennit()->rowCount(); i++) {
        debet += viennit()->data(viennit()->index(i, TositeViennit::DEBET), Qt::EditRole).toDouble();
        kredit += viennit()->data(viennit()->index(i, TositeViennit::KREDIT), Qt::EditRole).toDouble();
        if( viennit()->data( viennit()->index(i, TositeViennit::TILI), Qt::EditRole ).toInt() == 0)
            virheet |= TILIPUUTTUU;
    }
    if( qAbs(debet-kredit) > 1e-5 )
        virheet |= EITASMAA;
    if( qAbs(debet)  < 1e-5 && qAbs(kredit) < 1e-5)
        virheet |= NOLLA;

    emit tila(muutettu, virheet, debet, kredit);

    qDebug() << " M " << muutettu << " V " << virheet
             << "Debet " << debet << " Kredit " << kredit;
}

void Tosite::nollaa(const QDate &pvm, int tyyppi)
{
    data_.clear();
    data_.insert( avaimet__.at(PVM), pvm );
    data_.insert( avaimet__.at(TYYPPI), tyyppi);
    tallennettu_ = data_;
}

void Tosite::tallennusValmis(QVariant *variant)
{
    lataaData(variant);
    tallennuskesken_ = false;

    emit talletettu( data(ID).toInt(), data(TUNNISTE).toInt(), tallennettu_.value( avaimet__.at(PVM) ).toDate() );
    tarkasta();
}

void Tosite::tallennuksessaVirhe(int virhe)
{
    tallennuskesken_ = false;
    emit tallennusvirhe(virhe);
}

QVariantMap Tosite::tallennettava()
{
    QVariantMap map(data_);
    map.insert("viennit", viennit()->viennit());
    return map;
}


std::map<int,QString> Tosite::avaimet__ = {
    { ID, "id" },
    { PVM, "pvm"},
    { TYYPPI, "tyyppi"},
    { TILA, "tila"},
    { TUNNISTE, "tunniste"},
    { OTSIKKO, "otsikko"},
    { VIITE, "viite"},
    { ERAPVM, "erapvm"},
    { TOIMITTAJA, "toimittaja" },
    { ASIAKAS, "asiakas" },
    { INFO, "info"}
};
