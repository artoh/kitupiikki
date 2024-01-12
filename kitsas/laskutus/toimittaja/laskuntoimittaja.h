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
#ifndef LASKUNTOIMITTAJA_H
#define LASKUNTOIMITTAJA_H

#include <QWidget>
#include <QQueue>
#include <QPageLayout>

#include "model/tosite.h"
#include "abstraktitoimittaja.h"

namespace Ui {
    class LaskunToimittaja;
}

class LaskunTulostaja;
class QPainter;

class LaskunToimittaja : public QWidget
{
    Q_OBJECT
private:
    explicit LaskunToimittaja(QWidget *parent = nullptr);

public:
    static LaskunToimittaja* luoInstanssi(QWidget *parent = nullptr);
    static void toimita(const QList<int> laskuTositeIdt);

signals:

protected:

    void toimitaLasku(const QList<int> laskuTositeIdt);
    void laskuKasitelty();

    void toimitaSeuraava();
    void tositeLadattu();
    void laskuTallennettu();
    void liitteetLadattu();

    void tulosta(Tosite* tosite);

    void merkkaa(const int tositeId);

    void merkattu();
    void virhe(const QString virhe);
    void rekisteroiToimittaja(int tyyppi, AbstraktiToimittaja* toimittaja);

    void tyhjennaJono();

protected:
    Ui::LaskunToimittaja *ui;
    static LaskunToimittaja* instanssi__;

    QQueue<int> toimitettavaIdt_;
    bool kaynnissa_ = false;

    QSet<QString> virheet_;
    int onnistuneet_ = 0;
    int epaonnistuneet_ = 0;

    QPainter* painter_ = nullptr;
    QPageLayout vanhaleiska_;

    QMap<int, AbstraktiToimittaja*> toimittajat_;

};

#endif // LASKUNTOIMITTAJA_H
