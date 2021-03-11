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
#include "laskudialogitehdas.h"

#include "model/tosite.h"
#include "tavallinenlaskudialogi.h"
#include "db/tositetyyppimodel.h"
#include "db/kitsasinterface.h"



LaskuDialogiTehdas::LaskuDialogiTehdas(KitsasInterface *kitsas, QObject *parent) :
    QObject(parent),
    kitsas_(kitsas)
{

}

void LaskuDialogiTehdas::kaynnista(KitsasInterface *interface, QObject* parent)
{
    instanssi__ = new LaskuDialogiTehdas(interface, parent);
}

void LaskuDialogiTehdas::naytaLasku(int tositeId)
{
    Tosite* tosite = new Tosite(instanssi__);
    connect( tosite, &Tosite::ladattu, instanssi__, &LaskuDialogiTehdas::tositeLadattu);
    tosite->lataa(tositeId);
}

void LaskuDialogiTehdas::myyntilasku(int asiakasId)
{
    Tosite* tosite = new Tosite();
    tosite->asetaTyyppi(TositeTyyppi::MYYNTILASKU);
    tosite->asetaKumppani(asiakasId);
    tosite->asetaLaskupvm( paivamaara() );

    TavallinenLaskuDialogi *dlg = new TavallinenLaskuDialogi(tosite);
    dlg->show();
}

void LaskuDialogiTehdas::tositeLadattu()
{
    Tosite* tosite = qobject_cast<Tosite*>(sender());
    TavallinenLaskuDialogi* dlg = nullptr;

    if( tosite->tyyppi() == TositeTyyppi::MYYNTILASKU )
        dlg = new TavallinenLaskuDialogi(tosite);

    if( dlg )
        dlg->show();

}

QDate LaskuDialogiTehdas::paivamaara()
{
    return instanssi__->kitsas_ ? instanssi__->kitsas_->paivamaara() : QDate::currentDate();
}

LaskuDialogiTehdas* LaskuDialogiTehdas::instanssi__ = nullptr;
