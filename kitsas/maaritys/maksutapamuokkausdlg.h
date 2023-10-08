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
#ifndef MAKSUTAPAMUOKKAUSDLG_H
#define MAKSUTAPAMUOKKAUSDLG_H

#include <QDialog>
#include <QVariantMap>

#include "model/maksutapa.h"

namespace Ui {
class MaksutapaMuokkausDlg;
}

class MaksutapaModel;

class MaksutapaMuokkausDlg : public QDialog
{
    Q_OBJECT

public:
    explicit MaksutapaMuokkausDlg(QWidget *parent = nullptr);
    ~MaksutapaMuokkausDlg();

public:
    Maksutapa muokkaaMaksutapa(const Maksutapa &muokattava = Maksutapa());


private:
    Ui::MaksutapaMuokkausDlg *ui;    

};

#endif // MAKSUTAPAMUOKKAUSDLG_H
