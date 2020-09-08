/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "esikatselunaytin.h"
#include "esikatseltava.h"

#include <QPrinter>

Naytin::EsikatseluNaytin::EsikatseluNaytin(Esikatseltava *katseltava, QWidget *parent)
    : PrintPreviewNaytin (parent),
      esikatseltava_(katseltava)
{

}

Naytin::EsikatseluNaytin::~EsikatseluNaytin()
{

}

QString Naytin::EsikatseluNaytin::otsikko() const
{
    return esikatseltava_->otsikko();
}

QByteArray Naytin::EsikatseluNaytin::data() const
{
    return esikatseltava_->pdf();
}

void Naytin::EsikatseluNaytin::tulosta(QPrinter *printer) const
{
    esikatseltava_->tulosta(printer);
}

