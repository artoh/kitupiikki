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
#ifndef KPVIITEEDIT_H
#define KPVIITEEDIT_H

#include <QLineEdit>
#include "laskutus/viitenumero.h"


class ViiteValidator;

class KpViiteEdit : public QLineEdit
{
    Q_OBJECT
public:
    KpViiteEdit(QWidget* parent = nullptr);

    ViiteNumero viite();
    void setViite(const ViiteNumero& viite);
protected:
    void paintEvent(QPaintEvent *event) override;

    ViiteValidator* validator_;
};

#endif // KPVIITEEDIT_H
