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

#ifndef TOSITEWG_H
#define TOSITEWG_H

#include <QStackedWidget>
#include "ui_tositewg.h"


class QGraphicsScene;
class QGraphicsView;

class TositeWg : public QStackedWidget
{
    Q_OBJECT
public:
    TositeWg();
    ~TositeWg();

    QString tositeTunniste() const;
    bool onkoTiedostoa() const;

    bool tallennaTosite(int tositeId);

public slots:
    void lataaTosite(const QString& tositetiedostonpolku);
    void valitseTiedosto();

    void tyhjenna(const QString& tositenumero = QString(),
                  const QString& tositetiedosto = QString());


protected:
    Ui::TositeWg *ui;

    QGraphicsScene* scene;
    QGraphicsView *view;

    QString uusitiedostopolku;
    QString alkuperainentiedostopolku;

    void naytaTosite(const QString& tositetiedostonpolku);

};

#endif // TOSITEWG_H
