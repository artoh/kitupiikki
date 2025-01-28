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
#ifndef LASKURIVIDIALOGI_H
#define LASKURIVIDIALOGI_H

#include <QDialog>

#include "model/tositerivi.h"
#include "../laskualvcombo.h"

namespace Ui {
class LaskuRiviDialogi;
}

class KitsasInterface;

class LaskuRiviDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit LaskuRiviDialogi(QWidget *parent);
    ~LaskuRiviDialogi();

    void lataa(const TositeRivi& rivi, const QDate& pvm,
               LaskuAlvCombo::AsiakasVeroLaji asiakasVerolaji, bool ennakkolasku,
               KitsasInterface *interface);

    TositeRivi rivi();

protected:    
    void paivita();
    void maaraMuutos();
    void veroMuutos();

    void salliAlv(bool alv);

private:
    Ui::LaskuRiviDialogi *ui;

    TositeRivi rivi_;

    bool paivitys_ = false;
};

#endif // LASKURIVIDIALOGI_H
