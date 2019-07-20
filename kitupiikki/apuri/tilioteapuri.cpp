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
#include "tilioteapuri.h"
#include "ui_tilioteapuri.h"
#include "tiliotemodel.h"

#include "tiliotekirjaaja.h"
#include "model/tosite.h"

#include <QDate>

TilioteApuri::TilioteApuri(QWidget *parent, Tosite *tosite)
    : ApuriWidget (parent,tosite),
      ui( new Ui::TilioteApuri),
      model_(new TilioteModel(this))
{
    ui->setupUi(this);
    ui->oteView->setModel(model_);


    connect( ui->lisaaRiviNappi, &QPushButton::clicked, this, &TilioteApuri::lisaaRivi);
}

TilioteApuri::~TilioteApuri()
{

}

bool TilioteApuri::teeTositteelle()
{
    return false;
}

void TilioteApuri::teeReset()
{

}

void TilioteApuri::lisaaRivi()
{
    TilioteKirjaaja dlg(this);
    dlg.asetaPvm( tosite()->data(Tosite::PVM).toDate() );
    dlg.exec();
}

