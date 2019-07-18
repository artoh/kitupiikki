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
#include "asiakas.h"
#include "asiakastoimittajataydentaja.h"

#include "db/kirjanpito.h"

Asiakas::Asiakas(QObject *parent) : KantaAsiakasToimittaja (parent)
{

}

AsiakasToimittajaTaydentaja *Asiakas::taydentaja()
{
    if( !taydentaja_ ) {
        taydentaja_ = new AsiakasToimittajaTaydentaja(this);
        taydentaja_->lataa(AsiakasToimittajaTaydentaja::ASIAKKAAT);
    }
    return taydentaja_;
}

void Asiakas::lataa(QVariantMap data)
{
    data_ = data;
    muokattu_ = false;
}

void Asiakas::valitse(const QString &nimi)
{
    int id = taydentaja()->haeNimella(nimi);
    data_.clear();
    data_.insert("id", id);
    data_.insert("nimi", nimi);

    if( id > 0 ) {
        // Jos valitaan olemassa oleva, niin haetaan samalla
        // muutkin asiakkaan tiedot siltä varalta, että kohta
        // avataan valintaikkuna
        KpKysely* haku = kpk( QString("/asiakkaat/%1").arg(id));
        connect( haku, &KpKysely::vastaus,  [this] (QVariant* var) {this->lataa(var->toMap()); });
        haku->kysy();
    } else {
        muokattu_ = id == -1;
    }
}

void Asiakas::clear()
{
    data_.clear();
}

void Asiakas::tallenna(bool tositteentallennus)
{
    KpKysely* kysely;
    if( id() < 1)
        kysely = kpk( "/asiakkaat/", KpKysely::POST);
    else
        kysely = kpk( QString("/asiakkaat/%1").arg( id() ), KpKysely::PUT);

    if( tositteentallennus )
        connect(kysely, &KpKysely::vastaus, this, &Asiakas::tallennusvalmis  );
    else
        connect(kysely, &KpKysely::vastaus, this, &Asiakas::vaintallennusvalmis  );
    kysely->kysy( data_ );
}

void Asiakas::tallennusvalmis(QVariant *var)
{
    vaintallennusvalmis( var );
    emit tallennettu();
}

void Asiakas::vaintallennusvalmis(QVariant *var)
{
    QVariantMap map = var->toMap();
    data_.insert("id", map.value("id").toInt());
    muokattu_ = false;
}
