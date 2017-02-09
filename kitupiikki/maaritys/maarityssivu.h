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

#include "maarityswidget.h"

class QStackedWidget;
class QListWidget;
class QVBoxLayout;

/**
 * @brief Määritykset sisältävä QWidget
 */
class MaaritysSivu : public QWidget
{
    Q_OBJECT
public:
    MaaritysSivu();

signals:
    void nollaaKaikki();
public slots:
    void nollaa();

    void peru();
    void tallenna();

    void aktivoiSivu(int sivu);

protected:
    QListWidget *lista;
    QStackedWidget *pino;

    MaaritysWidget *nykyinen;
    QVBoxLayout *sivuleiska;

};

#endif // MAARITYSSIVU_H
