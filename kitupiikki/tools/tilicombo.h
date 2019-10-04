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
#ifndef TILICOMBO_H
#define TILICOMBO_H

#include <QComboBox>

class QSortFilterProxyModel;

class TiliCombo : public QComboBox
{
    Q_OBJECT
public:
    TiliCombo(QWidget *parent = nullptr);

    QSortFilterProxyModel *proxyTyyppi_;
    QSortFilterProxyModel *proxyTila_;

    void suodataTyypilla(const QString& regexp);
    void suodataMaksutapa(const QString& regexp);

    int valittuTilinumero() const;
    void valitseTili(int tilinumero);

private slots:
    void vaihtui();
    void valitseEka();

signals:
    void tiliValittu(int tilinumero);
};

#endif // TILICOMBO_H
