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
    return valittuna_;
}

QVariantMap EraCombo::eraMap() const
{
    int valittuna = currentData().toInt();
    if( !valittuna )
        return QVariantMap();
    if( valittuna == -1) {
        QVariantMap map;
        map.insert("id",-1);
        return map;
    }
    for( auto item : data_) {
        QVariantMap map = item.toMap();
        if( map.value("id").toInt() == valittuna)
            return map;
    }
    return QVariantMap();
}

void EraCombo::lataa(int tili, int asiakas)
{
    if( tili && (tili != tili_ || asiakas != asiakas_))
    {
        tili_ = tili;   // Jää välimuistiin jotta ei ladattaisi jatkuvasti
        asiakas_ = asiakas;
        KpKysely* kysely = kpk("/erat");
        if( kysely ) {
            kysely->lisaaAttribuutti("tili", QString::number(tili));
            if( asiakas > 0)
                kysely->lisaaAttribuutti("asiakas",QString::number(asiakas));

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
    else if( eraid > 0){
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
    latauksessa_ = true;
    clear();

    addItem( tr("Ei tase-erää"), 0);
    addItem(tr("Uusi tase-erä"), -1);

    QVariantList lista = data->toList();
    data_ = lista;

    for(auto item : lista) {
        QVariantMap map = item.toMap();
        int eraid = map.value("id").toInt();
        data_.append(map);

        QVariantMap kumppaniMap = map.value("kumppani").toMap();

        QString selite = map.value("selite").toString();
        QString kumppani = map.value("kumppani").toString();
        QString teksti = kumppani.isEmpty() || selite == kumppani ? selite : kumppani + " " + selite;

        addItem( QString("%1 %2 (%L3)")
                 .arg(map.value("pvm").toDate().toString("dd.MM.yyyy"))
                 .arg(teksti)
                 .arg(map.value("avoin").toDouble(),0,'f',2),
                 eraid);

        int indeksi = findData(eraid);
        setItemData( indeksi, map.value("avoin"), AvoinnaRooli );
        setItemData( indeksi, map.value("selite").toString(), SeliteRooli);
        if(kumppaniMap.value("id").toInt()) {
            setItemData( indeksi, kumppaniMap.value("id").toInt(), KumppaniRooli);
        }
    }
    setCurrentIndex( findData(valittuna_) );
    latauksessa_ = false;
}

void EraCombo::vientiSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    QString selite = map.value("selite").toString();
    QString kumppani = map.value("kumppani").toString();
    QString teksti = kumppani.isEmpty() || selite == kumppani ? selite : kumppani + " " + selite;

    addItem( QString("%1 %2")
             .arg(map.value("pvm").toDate().toString("dd.MM.yyyy"))
              .arg(teksti),
             map.value("id").toInt());
    setCurrentIndex( findData(valittuna_) );
}

void EraCombo::valintaMuuttui()
{

    if( !latauksessa_) {
        valittuna_ = currentData().toInt();
        emit valittu( valittuna_, currentData(AvoinnaRooli).toDouble(), currentData(SeliteRooli).toString(), currentData(KumppaniRooli).toInt() );
    }
}
