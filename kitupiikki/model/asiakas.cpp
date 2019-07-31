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
#include "db/kirjanpito.h"

Asiakas::Asiakas(QObject *parent) : KantaAsiakasToimittaja (parent)
{
}


void Asiakas::lataa(int id)
{
    KpKysely* haku = kpk( QString("/asiakkaat/%1").arg(id));
    connect( haku, &KpKysely::vastaus,  [this] (QVariant* var) {this->data_ = var->toMap(); emit this->ladattu(); });
    haku->kysy();
}

void Asiakas::clear()
{
    data_.clear();

}

void Asiakas::tallenna()
{
    KpKysely* kysely;
    if( id() < 1) {
        kysely = kpk( "/asiakkaat/", KpKysely::POST);
        data_.remove("id");
    } else
        kysely = kpk( QString("/asiakkaat/%1").arg( id() ), KpKysely::PUT);

    connect(kysely, &KpKysely::vastaus, this, &Asiakas::tallennusvalmis  );

    kysely->kysy( data_ );
}

