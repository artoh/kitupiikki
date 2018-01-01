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

#ifndef TILINVALINTAVIEW_H
#define TILINVALINTAVIEW_H

#include <QTableView>

/**
 * @brief Tilinvalinnassa käytettävä view, joka kertoo hiiren alla olevan tiliId:n
 *
 * Jotta tilinvalintadialogi voisi näyttää kirjausohjeen, seuraa tämä view hiirtä ja ilmoittaa
 * tiliHiirenAlla(int tiliId)-signaalilla, mikä tili on hiiren alla
 *
 */
class TilinValintaView : public QTableView
{
    Q_OBJECT
public:
    TilinValintaView(QWidget *parent = nullptr);

    void paivitaInfo();

signals:
    void tiliHiirenAlla(int tiliId);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;

    int ilmoitettuTili;
};

#endif // TILINVALINTAVIEW_H
