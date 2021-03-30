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
#ifndef LASKUNUUSINTA_H
#define LASKUNUUSINTA_H

#include <QObject>
#include <QTimer>
#include <QQueue>

class Tosite;

class LaskunUusinta : public QObject
{
    Q_OBJECT
public:
    LaskunUusinta(QObject *parent = nullptr);

signals:

private:
    void ajastaUusita();
    void uusiLaskut();
    void listaSaapuu(QVariant* lista);
    void uusiSeuraava();
    void uusittavaLadattu();
    void paivitaHinnat();
    void asiakasSaapuu(QVariant* asiakas);
    void jatkaTallentamaan();
    void laskuUusittu();

    QTimer timer_;
    QQueue<int> jono_;

    Tosite* tosite_;
    Tosite* uusi_;
    bool busy_ = false;
};

#endif // LASKUNUUSINTA_H
