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

#ifndef ALVILMOITUSDIALOG_H
#define ALVILMOITUSDIALOG_H

#include <QDialog>

namespace Ui {
class AlvIlmoitusDialog;
}

class AlvIlmoitusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AlvIlmoitusDialog(QWidget *parent = 0);
    ~AlvIlmoitusDialog();

    static QDate teeAlvIlmoitus(QDate alkupvm, QDate loppupvm);

private:
    Ui::AlvIlmoitusDialog *ui;
};

#endif // ALVILMOITUSDIALOG_H
