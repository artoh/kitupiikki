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
#ifndef TILIOTEKIRJAAJA_H
#define TILIOTEKIRJAAJA_H

#include <QDialog>

#include "tiliotemodel.h"

class LaskuTauluTilioteProxylla;
class TilioteApuri;

class QSortFilterProxyModel;

namespace Ui {
class TilioteKirjaaja;
}

class TilioteKirjaaja : public QDialog
{
    Q_OBJECT    
public:
    enum AlaTab { MAKSU, TULOMENO, SIIRTO, PIILOSSA };

    TilioteKirjaaja( TilioteApuri* apuri );
    ~TilioteKirjaaja() override;

    void asetaPvm(const QDate& pvm);

    TilioteModel::Tilioterivi rivi();

public slots:
    void accept() override;
    void muokkaaRivia(int riviNro);

private slots:
    void alaTabMuuttui(int tab);
    void euroMuuttuu();
    void ylaTabMuuttui(int tab);
    void tiliMuuttuu();
    void eraValittu(int eraId, double avoinna, const QString& selite);
    void jaksomuuttuu(const QDate& pvm);

    void valitseLasku();
    void suodata(const QString& teksti);

    void tyhjenna();
    void tarkastaTallennus();

    void kumppaniValittu(int kumppaniId);
    void kumppaniTiedot(QVariant* data);


private:
    TilioteApuri* apuri();

private:
    Ui::TilioteKirjaaja *ui;

    int menoa_ = false;

    QSortFilterProxyModel* maksuProxy_;    
    QSortFilterProxyModel* avoinProxy_;

    LaskuTauluTilioteProxylla *laskut_;

    int muokattavaRivi_ = -1;
};

#endif // TILIOTEKIRJAAJA_H
