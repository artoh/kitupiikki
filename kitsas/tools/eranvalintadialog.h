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
#ifndef ERANVALINTADIALOG_H
#define ERANVALINTADIALOG_H

#include <QDialog>

class QAbstractItemModel;
class EraProxyModel;

namespace Ui {
class EranValintaDialog;
}

class EranValintaDialog : public QDialog
{
    Q_OBJECT

public:    
    explicit EranValintaDialog(QWidget *parent = nullptr);
    virtual ~EranValintaDialog();

    virtual QVariantMap valittu() const = 0;

    void paivitaSuodatus();
    void paivitaOk();

protected:
    void asetaModel(QAbstractItemModel* model);
    virtual void paivitaNykyinen() = 0;

protected:
    Ui::EranValintaDialog *ui;
    EraProxyModel* proxy_;
};

#endif // ERANVALINTADIALOG_H
