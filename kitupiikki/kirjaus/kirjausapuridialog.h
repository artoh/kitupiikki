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

#ifndef KIRJAUSAPURIDIALOG_H
#define KIRJAUSAPURIDIALOG_H

#include <QDialog>

namespace Ui {
class KirjausApuriDialog;
}

class KirjausApuriDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KirjausApuriDialog(QWidget *parent = 0);
    ~KirjausApuriDialog();

private:
    Ui::KirjausApuriDialog *ui;
};

#endif // KIRJAUSAPURIDIALOG_H
