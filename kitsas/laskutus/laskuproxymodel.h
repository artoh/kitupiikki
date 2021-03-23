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
#ifndef LASKUPROXYMODEL_H
#define LASKUPROXYMODEL_H

#include <QSortFilterProxyModel>

class LaskuProxyModel : public QSortFilterProxyModel
{
public:
    LaskuProxyModel(QObject* parent = nullptr);

    void suodataNumerolla(const QString& numero);
    void suodataTekstilla(const QString& teksti);
    void suodataViittella(const QString& viite);
    void suodataLaskut(bool vainLaskut);
    void suodataKumppani(int kumppani);
    void suodataViiteTyypilla(int viitetyyppi);
    void nollaaSuodatus();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    int kumppani_ = 0;
    QString numero_;
    QString teksti_;
    QString viite_;
    bool vainLaskut_ = false;
    int viitetyyppi_ = 0;


};

#endif // LASKUPROXYMODEL_H
