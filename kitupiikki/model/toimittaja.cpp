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
#include "toimittaja.h"
#include "asiakastoimittajataydentaja.h"
#include "db/kirjanpito.h"

#include "validator/ibanvalidator.h"

Toimittaja::Toimittaja(QObject *parent) : KantaAsiakasToimittaja (parent)
{

}

void Toimittaja::lataa(QVariantMap data)
{
    data_ = data;
    tilit_.clear();
    QVariantList tiliList = data_.take("iban").toList();
    for( auto tili : tiliList)
        tilit_.append( tili.toString() );
}



void Toimittaja::setTilit(const QStringList &lista)
{
    tilit_.clear();
    for( auto tili : lista) {
        if( IbanValidator::kelpaako(tili) )
            tilit_.append(tili.remove(' '));
    }
}

AsiakasToimittajaTaydentaja *Toimittaja::taydentaja()
{
    if( !taydentaja_ ) {
        taydentaja_ = new AsiakasToimittajaTaydentaja(this);
        taydentaja_->lataa(AsiakasToimittajaTaydentaja::TOIMITTAJAT);
    }
    return taydentaja_;
}

void Toimittaja::valitse(const QString &nimi)
{
    int id = taydentaja()->haeNimella(nimi);
    data_.clear();
    data_.insert("id", id);
    data_.insert("nimi", nimi);

    if( id > 0 ) {
        // Jos valitaan olemassa oleva, niin haetaan samalla
        // muutkin asiakkaan tiedot siltä varalta, että kohta
        // avataan valintaikkuna
        KpKysely* haku = kpk( QString("/toimittajat/%1").arg(id));
        connect( haku, &KpKysely::vastaus,  [this] (QVariant* var) {this->lataa(var->toMap()); });
        haku->kysy();
    } else {
        muokattu_ = id == -1;
    }
}
