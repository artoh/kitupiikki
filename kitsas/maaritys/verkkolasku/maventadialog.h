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
#ifndef MAVENTADIALOG_H
#define MAVENTADIALOG_H

#include <QDialog>

namespace Ui {
    class MaventaDialog;
};

class MaventaDialog : public QDialog
{
    Q_OBJECT
public:
    MaventaDialog(QWidget *parent = nullptr);

    void accept() override;
    void lataa(const QVariantMap& data);

    static QVariantMap settingsAsetuksista();

signals:
    void liitetty(QVariant* info);

private:
    void vastaus(QVariant* data);
    void virhe(int koodi);
    void muokattu();

    Ui::MaventaDialog *ui;
};

#endif // MAVENTADIALOG_H
