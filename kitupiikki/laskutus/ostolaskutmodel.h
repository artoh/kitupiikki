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

#ifndef OSTOLASKUTMODEL_H
#define OSTOLASKUTMODEL_H

#include "laskutmodel.h"

/**
 * @brief Ostolaskujen luettelon model
 *
 * Tätä käytetään laskujen maksun dialogissa, jotta voidaan käsitellä myös
 * ostolaskuja
 */
class OstolaskutModel : public LaskutModel
{
    Q_OBJECT
public:

    OstolaskutModel(QObject *parent=0);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public slots:
    void lataaAvoimet();


};

#endif // OSTOLASKUTMODEL_H
