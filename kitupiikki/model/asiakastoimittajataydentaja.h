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
#ifndef ASIAKASTOIMITTAJATAYDENTAJA_H
#define ASIAKASTOIMITTAJATAYDENTAJA_H

#include <QAbstractListModel>

class AsiakasToimittajaTaydentaja : public QAbstractListModel
{
    Q_OBJECT  

public:
    explicit AsiakasToimittajaTaydentaja(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    enum AsiakasVaiToimittaja {
        TOIMITTAJAT = 0,
        ASIAKKAAT = 1
    };

    int haeNimella(const QString& nimi) const;

public slots:
    void lataa(AsiakasVaiToimittaja avt);

private slots:
    void saapuu(QVariant* variant);

private:
    QList<QPair<QString,int>> data_;
};

#endif // ASIAKASTOIMITTAJATAYDENTAJA_H
