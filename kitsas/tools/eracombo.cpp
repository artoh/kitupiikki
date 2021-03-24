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

#include "laskutus/viitenumero.h"

#include "rekisteri/asiakastoimittajalistamodel.h"
#include "huoneistoeranvalintadialog.h"
#include "eraeranvalintadialog.h"

EraCombo::EraCombo(QWidget *parent) :
    QComboBox (parent)
{
    connect( this, qOverload<int>(&EraCombo::currentIndexChanged),
             this, &EraCombo::vaihtui);
}

int EraCombo::valittuEra() const
{
    return eraId_;
}

QVariantMap EraCombo::eraMap() const
{
    QVariantMap map;
    map.insert("id", eraId_);
    if(!eraNimi_.isEmpty())
        map.insert("selite", eraNimi_);
    if( eraSaldo_.cents() && eraId_ > 0)
        map.insert("avoin", eraSaldo_.toString());
    return map;
}

void EraCombo::asetaTili(int tili, int asiakas)
{
    tili_ = tili;
    asiakas_ = asiakas;
    asiakasNimi_ = asiakas ? AsiakasToimittajaListaModel::instanssi()->nimi(asiakas) : QString();
    paivita();
}

void EraCombo::valitseUusiEra()
{
    eraId_ = -1;
}

void EraCombo::valitse(const QVariantMap &eraMap)
{
    eraId_ = eraMap.value("id").toInt();
    eraNimi_ = eraMap.value("selite").toString();

    asiakas_ = eraMap.value("asiakas").toMap().value("id").toInt();
    asiakasNimi_ = eraMap.value("asiakas").toMap().value("nimi").toString();

    eraSaldo_ = Euro::fromVariant(eraMap.value("avoin"));
    eranPaiva_ = eraMap.value("pvm").toDate();
    paivita();
}

void EraCombo::paivita()
{
    paivitetaan_ = true;
    clear();

    if( eraId_ > 0 ) {
        if( eraSaldo_.cents()) {
            addItem(QIcon(":/pic/lasku.png"), QString("%1 %2 %3 (%4)")
                    .arg(eranPaiva_.toString("dd.MM.yyyy"))
                    .arg(eraNimi_)
                    .arg(asiakasNimi_)
                    .arg(eraSaldo_.display()), eraId_);
        } else {
            addItem(QIcon(":/pic/ok.png"), QString("%1 %2 %3")
                    .arg(eranPaiva_.toString("dd.MM.yyyy"))
                    .arg(eraNimi_)
                    .arg(asiakasNimi_), eraId_);
        }
    } else if( eraId_ < -10) {
        if( eraId_ % 10 == -3) {
            addItem(QIcon(":/pic/mies.png"), eraNimi_, eraId_);
        } else if( eraId_ % 10 == -4) {
            addItem(QIcon(":/pic/talo.png"), eraNimi_, eraId_);
        }
    }

    addItem(QIcon(":/pic/tyhja.png"), tr("Ei tase-erää"), 0);
    addItem(QIcon(":/pic/lisaa.png"), tr("Uusi tase-erä"), -1);
    addItem(QIcon(":/pic/lasku.png"), tr("Valitse tase-erä"), -2);

    if( asiakas_ ) {
        QString nimi = AsiakasToimittajaListaModel::instanssi()->nimi(asiakas_);
        addItem(QIcon(":/pic/mies.png"), nimi, -3);
    }
    addItem(QIcon(":/pic/talo.png"), tr("Huoneisto"), -4);


    setCurrentIndex( findData(eraId_) );

    paivitetaan_ = false;
}

void EraCombo::vaihtui()
{
    int indeksi = currentIndex();
    if( indeksi < 0 || paivitetaan_) {
        return;
    }
    int vanhaEra = eraId_;

    int era = currentData().toInt();
    if( era == -1) {
      eraId_ = -1;
      eraSaldo_ = 0;
      eraNimi_.clear();
      asiakas_ = 0;
    } else if(era == 0) {
      eraId_ = 0;
      eraNimi_.clear();
      eraSaldo_ = 0;
      asiakas_ = 0;
    } else if( era == -2) {
        EraEranValintaDialog dlg(tili_, asiakas_, eraId_, this);
        if( dlg.exec() == QDialog::Accepted) {
            valitse( dlg.valittu());
        }

    } else if( era == -3) {
        ViiteNumero hloViite(ViiteNumero::ASIAKAS, asiakas_);
        eraId_ = hloViite.eraId();
        eraNimi_ = asiakasNimi_;
        eraSaldo_ = 0;
    } else if( era == -4) {
        HuoneistoEranValintaDialog dlg(eraId_ / -10, this);
        if( dlg.exec() == QDialog::Accepted) {
            valitse( dlg.valittu() );
        }
    }

    if( vanhaEra != eraId_)
        emit valittu(eraId_, eraSaldo_, eraNimi_, asiakas_);

    paivita();
}


