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
#ifndef TILIOTEAPURI_H
#define TILIOTEAPURI_H

#include "../apuriwidget.h"

class TilioteModel;

namespace Ui {
class TilioteApuri;
}

class TilioteKirjaaja;

class TilioteApuri : public ApuriWidget
{
    Q_OBJECT
public:
    TilioteApuri(QWidget *parent = nullptr, Tosite* tosite = nullptr);
    virtual ~TilioteApuri() override;

    void tuo(QVariantMap map) override;

    TilioteModel* model() { return model_;}

    QDate tiliotteenAlkupaiva() const;
    QDate tiliotteenLoppupaiva() const;

public slots:
     void salliMuokkaus(bool sallitaanko=true) override;

protected:
    bool teeTositteelle() override;
    void teeReset() override;

protected slots:
    void lisaaRivi(bool dialogi = false);

    void riviValittu();
    void muokkaa();
    void poista();
    void naytaSummat();
    void naytaTosite();

    void tiliPvmMuutos();
    void lataaHarmaat();
    void laitaPaivat(const QDate& pvm);

    void kysyAlkusumma();
    void alkusummaSaapuu(QVariant *data);
    void naytaHarmaat(bool nayta);

private:
    Ui::TilioteApuri *ui;
    TilioteModel *model_;
    TilioteKirjaaja *kirjaaja_;

    QSortFilterProxyModel *proxy_;
    double alkusaldo_;
    bool tuodaan_ = false;
    bool paivanlaitto_ = false;
};

#endif // TILIOTEAPURI_H
