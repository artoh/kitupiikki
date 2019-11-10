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
#include "eracombo.h"
#include "db/kirjanpito.h"

#include <QDebug>

EraCombo::EraCombo(QWidget *parent) :
    QComboBox (parent)
{
    connect( this, &QComboBox::currentTextChanged, this, &EraCombo::valintaMuuttui);
}

int EraCombo::valittuEra() const
{
    return currentData().toInt();
}

void EraCombo::lataa(int tili)
{
    if( tili )
    {
        KpKysely* kysely = kpk("/erat");
        if( kysely ) {
            kysely->lisaaAttribuutti("tili", QString::number(tili));

            connect(kysely, &KpKysely::vastaus, this, &EraCombo::dataSaapuu);
            kysely->kysy();
        }
    }
}

void EraCombo::valitse(int eraid)
{
    valittuna_ = eraid;

    int indeksi = findData( valittuna_ );
    if( indeksi > -1)
        setCurrentIndex( findData(valittuna_) );
    else {
        // Tilataan erän tiedot
        KpKysely* kysely = kpk(QString("/viennit/%1").arg(eraid));
        if( kysely ) {
            connect(kysely, &KpKysely::vastaus, this, &EraCombo::vientiSaapuu);
            kysely->kysy();
        }
    }
}

void EraCombo::dataSaapuu(QVariant *data)
{
    clear();

    addItem( tr("Ei tase-erää"), 0);
    addItem(tr("Uusi tase-erä"), -1);

    QVariantList lista = data->toList();
    for(auto item : lista) {
        QVariantMap map = item.toMap();

        addItem( QString("%1 %2 (%3)")
                 .arg(map.value("pvm").toDate().toString("dd.MM.yyyy"))
                 .arg(map.value("selite").toString())
                 .arg(map.value("avoin").toDouble(),0,'f',2),
                 map.value("eraid").toInt());

        setItemData( findData(map.value("eraid").toInt()), map.value("avoin"), AvoinnaRooli );
    }
    setCurrentIndex( findData(valittuna_) );
}

void EraCombo::vientiSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    addItem( QString("%1 %2")
             .arg(map.value("pvm").toDate().toString("dd.MM.yyyy"))
              .arg(map.value("selite").toString()  ),
             map.value("id").toInt());
    setCurrentIndex( findData(valittuna_) );
}

void EraCombo::valintaMuuttui()
{
    valittuna_ = currentData().toInt();
    emit valittu( valittuna_, currentData(AvoinnaRooli).toDouble() );
}
