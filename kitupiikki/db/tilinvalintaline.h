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

#ifndef TILINVALINTALINE_H
#define TILINVALINTALINE_H

#include <QLineEdit>
#include <QModelIndex>
#include "kirjanpito.h"
#include "vientimodel.h"

/**
 * @brief QLineEditor, joka valitsee tilejä delegaatille
 *
 * Tämä säädetty delegaatin käyttöön: jos pitäisi mennä
 * valintaikkunaan, heittää fokuksen vanhemmalle
 *
 */
class TilinvalintaLineDelegaatille : public QLineEdit
{
    Q_OBJECT
public:
    TilinvalintaLineDelegaatille(QWidget *parent = 0);

    void valitseTiliNumerolla(int tilinumero);
    int valittuTilinumero() const;

    QString tilinimiAlkaa() const { return alku_; }

protected:
    void keyPressEvent(QKeyEvent *event);

public slots:
    void valitseTili(Tili tili);

protected:
    QString alku_;

};

#endif // TILINVALINTALINE_H
