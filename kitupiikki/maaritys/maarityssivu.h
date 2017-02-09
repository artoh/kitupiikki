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

#ifndef MAARITYSSIVU_H
#define MAARITYSSIVU_H

#include <QWidget>
#include <QIcon>

#include "maarityswidget.h"
#include "kitupiikkisivu.h"

class QStackedWidget;
class QListWidget;
class QVBoxLayout;
class QListWidgetItem;

/**
 * @brief Määrityssivun sisältävä QWidget
 */
class MaaritysSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:

    enum Sivut
    {
        PERUSVALINNAT,
        TILINAVAUS,
        TOSITELAJIT
    };


    MaaritysSivu();

signals:
    void nollaaKaikki();
public slots:
    void nollaa();

    void peru();
    void tallenna();

    void aktivoiSivu(int sivu);

    void valitseSivu( QListWidgetItem *item);

protected:
    void lisaaSivu(const QString& otsikko, Sivut sivu, const QIcon& kuvake = QIcon());

protected:
    QListWidget *lista;
    QStackedWidget *pino;

    MaaritysWidget *nykyinen;
    QListWidgetItem *nykyItem;
    QVBoxLayout *sivuleiska;

};

#endif // MAARITYSSIVU_H
