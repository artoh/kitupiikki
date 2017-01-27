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

#ifndef TILINAVAUS_H
#define TILINAVAUS_H

#include <QWidget>

#include "ui_tilinavaus.h"
#include "tilinavausmodel.h"

class Tilinavaus : public QWidget
{
    Q_OBJECT
public:
    explicit Tilinavaus(QWidget *parent = 0);
    ~Tilinavaus();

signals:

public slots:
    void naytaInfo(QString info);
    void tsekkaaMuokkaus();

private:
    Ui::Tilinavaus *ui;
    TilinavausModel *model;
};

#endif // TILINAVAUS_H
