/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef TOSITELAJIDIALOGI_H
#define TOSITELAJIDIALOGI_H

#include <QDialog>

#include <QModelIndex>
#include "db/tositelajimodel.h"

namespace Ui {
class TositelajiDialogi;
}

/**
 * @brief Tositelajin muokkaamisen dialogi
 */
class TositelajiDialogi : public QDialog
{
    Q_OBJECT

public:
     TositelajiDialogi(TositelajiModel *model,
             const QModelIndex& index = QModelIndex(), QWidget *parent = 0);
    ~TositelajiDialogi();

protected slots:
    void lataa();
    void tarkasta();
    void vastatilivalittu();

    void accept();

private:
    Ui::TositelajiDialogi *ui;

    TositelajiModel *model_;
    QModelIndex indeksi_;
};

#endif // TOSITELAJIDIALOGI_H
