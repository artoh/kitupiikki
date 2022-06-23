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
#ifndef UUSIMAKSUMUISTUTUSDIALOGI_H
#define UUSIMAKSUMUISTUTUSDIALOGI_H

#include <QDialog>
#include <QMap>

namespace Ui {
class MaksumuistutusDialogi;
}

class UusiMaksumuistutusDialogi : public QDialog
{
    Q_OBJECT

public:
    UusiMaksumuistutusDialogi(QVariantList muistutuslista, QWidget *parent = nullptr);
    ~UusiMaksumuistutusDialogi() override;

    void kaynnista();
    void accept() override;

signals:
    void muistutettu();

protected:
    void eraSaapuu(QVariant* data);
    void tositeSaapuu(QVariant* data);
    void tallennaMuistutus();

    void tallennaSeuraava();
    void merkkaaMuistutetuksi(const QVariantMap& data);

private:
    Ui::MaksumuistutusDialogi *ui;

    QVariantList lista_;
    int nykyinen_ = -1;
    int tositteita_ = 0;
};

#endif // UUSIMAKSUMUISTUTUSDIALOGI_H
