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
#ifndef MAKSUMUISTUTUSDIALOGI_H
#define MAKSUMUISTUTUSDIALOGI_H

#include <QDialog>
#include <QMap>

namespace Ui {
class MaksumuistutusDialogi;
}

class MaksumuistutusDialogi : public QDialog
{
    Q_OBJECT

public:
    MaksumuistutusDialogi(QList<int> erat, QWidget *parent = nullptr);
    ~MaksumuistutusDialogi() override;

    void accept() override;

protected:
    void haeEra();
    void eraSaapuu(QVariant* data);
    void tositeSaapuu(int era, QVariant* data);
    QVariantMap muodostaMuistutus(int era);

    void tallennaSeuraava();
    void tallennaLiite(QVariant* data);
    void merkkaaMuistutetuksi(const QVariantMap& data);

private:
    Ui::MaksumuistutusDialogi *ui;

    QList<int> erat_;
    QMap<int,QVariantList> muistutettavat_;
    QMap<int,QVariantMap> eraMapit_;
    QList<QVariantMap> muistutukset_;
};

#endif // MAKSUMUISTUTUSDIALOGI_H
