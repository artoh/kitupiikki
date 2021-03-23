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

#include "model/tosite.h"
#include "abstraktitoimittaja.h"

namespace Ui {
    class onniWidget;
}

class LaskunTulostaja;

class LaskunToimittaja : public QWidget
{
    Q_OBJECT
private:
    explicit LaskunToimittaja(QWidget *parent = nullptr);

public:
    static LaskunToimittaja* luoInstanssi(QWidget *parent = nullptr);
    static void toimita(const QVariantMap& tosite);
    static void toimita(const int id);

    void peru();
signals:

protected:
    void toimitaLasku(const QVariantMap& tosite);
    void toimitaLasku(const int tositeid);

    void haeLasku();
    void laskuSaapuu(QVariant* data);
    void tallennaLiite();
    void liiteTallennettu();

    void silmukka();

    void onnistui();
    void virhe(const QString virhe);
    void tarkastaValmis();

    void rekisteroiToimittaja(int tyyppi, AbstraktiToimittaja* toimittaja);

protected:
    Ui::onniWidget *ui;

    bool noutoKaynnissa_ = false;
    bool liiteKaynnissa_ = false;

    static LaskunToimittaja* instanssi__;

    QQueue<int> haettavat_;
    QQueue<QVariantMap> tositteet_;

    QSet<QString> virheet_;
    int onnistuneet_ = 0;
    int epaonnistuneet_ = 0;

    QMap<int, AbstraktiToimittaja*> toimittajat_;

    LaskunTulostaja* tulostaja_;

};

#endif // LASKUNTOIMITTAJA_H
