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
#include "db/kirjanpito.h"

#include "validator/ibanvalidator.h"

Toimittaja::Toimittaja(QObject *parent) : KantaAsiakasToimittaja (parent)
{
}



void Toimittaja::setTilit(const QStringList &lista)
{
    tilit_.clear();
    for( auto tili : lista) {
        if( IbanValidator::kelpaako(tili) )
            tilit_.append(tili.remove(' '));
    }
}

void Toimittaja::lataa(int id)
{
    KpKysely* haku = kpk( QString("/toimittajat/%1").arg(id));
    connect( haku, &KpKysely::vastaus,  [this] (QVariant* var) {this->data_ = var->toMap(); emit this->ladattu(); });
    haku->kysy();
}



void Toimittaja::clear()
{
    data_.clear();
    tilit_.clear();
}

void Toimittaja::tallenna()
{
    KpKysely* kysely;
    if( id() < 1) {
        kysely = kpk( "/toimittajat/", KpKysely::POST);
        data_.remove("id");
    } else
        kysely = kpk( QString("/toimittajat/%1").arg( id() ), KpKysely::PUT);

    connect(kysely, &KpKysely::vastaus, this, &Toimittaja::tallennusvalmis  );

    data_.insert("iban", tilit_);
    kysely->kysy( data_ );
}
