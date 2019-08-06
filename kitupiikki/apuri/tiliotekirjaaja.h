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

class KohdennusProxyModel;
class LaskuTauluModel;

class QSortFilterProxyModel;

namespace Ui {
class TilioteKirjaaja;
}

class TilioteKirjaaja : public QDialog
{
    Q_OBJECT    
public:
    enum AlaTab { MAKSU, TULOMENO, SIIRTO, PIILOSSA };

    TilioteKirjaaja(  QWidget *parent = nullptr, TilioteModel::Tilioterivi rivi = TilioteModel::Tilioterivi());
    ~TilioteKirjaaja();

    void asetaPvm(const QDate& pvm);

    TilioteModel::Tilioterivi rivi();

private slots:
    void alaTabMuuttui(int tab);
    void euroMuuttuu();
    void ylaTabMuuttui(int tab);

    void valitseLasku();
    void suodata(const QString& teksti);

private:
    Ui::TilioteKirjaaja *ui;

    int menoa_ = false;

    TilioteModel::Tilioterivi rivi_;
    KohdennusProxyModel* kohdennusProxy_;
    QSortFilterProxyModel* maksuProxy_;

    LaskuTauluModel *laskut_;
};

#endif // TILIOTEKIRJAAJA_H
