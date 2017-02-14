/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "tositelajit.h"
#include "db/kirjanpito.h"


Tositelajit::Tositelajit(QWidget *parent) : MaaritysWidget(parent)
{
    ui = new Ui::Tositelajit;
    ui->setupUi(this);

    model = new TositelajiModel( kp()->tietokanta(), this);
    ui->view->setModel(model);

    connect(ui->uusiNappi, SIGNAL(clicked(bool)), model, SLOT(lisaaRivi()));
}

Tositelajit::~Tositelajit()
{
    delete ui;
}

bool Tositelajit::tallenna()
{
    // Tallentaa nämä tietokantaan
    model->tallenna();
    // Lataa kirjanpidon modelin
    kp()->tositelajit()->lataa();
    return true;
}

bool Tositelajit::nollaa()
{
    model->lataa();
    return true;
}
