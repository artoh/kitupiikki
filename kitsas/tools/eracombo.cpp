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
#include "laskutus/huoneisto/huoneistomodel.h"
#include "db/tositetyyppimodel.h"
#include "pilvi/pilvimodel.h"

EraCombo::EraCombo(QWidget *parent) :
    QComboBox (parent)
{
    connect( this, qOverload<int>(&EraCombo::currentIndexChanged),
             this, &EraCombo::vaihtui);
}

int EraCombo::valittuEra() const
{
    return era_.value("id").toInt();
}

EraMap EraCombo::eraMap() const
{
    return era_;
}

void EraCombo::asetaTili(int tili, int asiakas)
{
    if( tili != tili_ || asiakas != asiakas_) {
        tili_ = tili;
        asiakas_ = asiakas;
        asiakasNimi_ = asiakas ? AsiakasToimittajaListaModel::instanssi()->nimi(asiakas) : QString();
        paivita();
    }
}

void EraCombo::valitseUusiEra()
{
    era_ = EraMap(EraMap::Uusi);
    paivita();
}

void EraCombo::valitseEiEraa()
{
    era_ = EraMap(EraMap::EiEraa);
    paivita();
}

void EraCombo::valitse(const EraMap &eraMap)
{
    era_ = eraMap;
    paivita();
}

void EraCombo::paivita()
{
    paivitetaan_ = true;
    clear();


    if( era_.eratyyppi() == EraMap::Lasku ) {
        QString txt = QString("%1 %2 %3")
                .arg(era_.pvm().toString("dd.MM.yyyy"), era_.nimi(), era_.kumppaniNimi());

        if( era_.saldo() ) {
            txt.append(" (" + era_.saldo().display() + ")");
        }

        addItem( era_.saldo() || era_.tunniste() == 0 ? kp()->tositeTyypit()->kuvake(era_.tositetyyppi())
                       : QIcon(":/pic/ok.png"),
                 txt,
                 era_ );
    } else if( era_.eratyyppi() == EraMap::Asiakas) {
        addItem(QIcon(":/pic/mies.png"), asiakasNimiIdlla(era_.id() ), era_);
    } else if( era_.eratyyppi() == EraMap::Huoneisto) {
        addItem(QIcon(":/pic/talo.png"),  huoneistoNimiIdlla(era_.id()) ,era_);
    }

    addItem(QIcon(":/pic/huomio.png"), tr("Ei tase-erää (Erittelemätön)"), EraMap(EraMap::EiEraa));
    addItem(QIcon(":/pic/lisaa.png"), tr("Uusi tase-erä"), EraMap(EraMap::Uusi));
    addItem(QIcon(":/pic/lasku.png"), tr("Valitse tase-erä"), EraMap(EraMap::Valitse));

    const Tili* tili = kp()->tilit()->tili(tili_);
    const bool myyntisaamista = tili && tili->onko(TiliLaji::MYYNTISAATAVA);

    if( asiakas_ && qobject_cast<PilviModel*>(kp()->yhteysModel()) && myyntisaamista) {
        addItem(QIcon(":/pic/mies.png"),  asiakasNimi_, EraMap::AsiakasEra(asiakas_, asiakasNimi_) );
    }
    if( kp()->huoneistot()->rowCount() && myyntisaamista)
        addItem(QIcon(":/pic/talo.png"), tr("Huoneisto"), EraMap(EraMap::Huoneisto));

    int indeksi = findData(era_);
    setCurrentIndex( indeksi > -1 ? indeksi : 0 );

    paivitetaan_ = false;
}

void EraCombo::vaihtui()
{
    int indeksi = currentIndex();
    if( indeksi < 0 || paivitetaan_) {
        return;
    }

    int vanhaId = era_.id();
    EraMap uusi = currentData().toMap();

    if( uusi.id() == EraMap::Valitse ) {
        EraEranValintaDialog dlg(tili_, asiakas_, era_.id(), this);
        if( dlg.exec() == QDialog::Accepted) {
            valitse( dlg.valittu());
        }     
    } else if( uusi.id() == EraMap::Huoneisto) {
        HuoneistoEranValintaDialog dlg(era_.huoneistoId(), this);
        if( dlg.exec() == QDialog::Accepted) {
            valitse( dlg.valittu() );
        }
    } else {
        era_ = uusi;
    }

    if( vanhaId != era_.id())
        emit valittu( era_);

    paivita();
}

QString EraCombo::asiakasNimiIdlla(int era) const
{
    return AsiakasToimittajaListaModel::instanssi()->nimi( era / -10);
}

QString EraCombo::huoneistoNimiIdlla(int era) const
{
    return kp()->huoneistot()->tunnus( era / -10 );
}



