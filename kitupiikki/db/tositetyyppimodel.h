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
#ifndef TOSITETYYPPIMODEL_H
#define TOSITETYYPPIMODEL_H

#include <QAbstractListModel>
#include <QIcon>

namespace TositeTyyppi {
    enum Tyyppi
    {
        MUU = 0,
        MENO = 100,
        TULO = 200,
        SIIRTO = 300,
        TILIOTE = 400,
        PALKKA = 500,
        MUISTIO = 700,
        LIITETIETO = 800,
        TILINAVAUS = 901,
        ALVLASKELMA = 910,
        POISTOLASKELMA = 991,
        JAKSOTUS = 992
    };
}


struct TositeTyyppiTietue
{
    TositeTyyppiTietue() {;}
    TositeTyyppiTietue(TositeTyyppi::Tyyppi uKoodi, const QString& uNimi, const QString& uKuvake = QString());

    TositeTyyppi::Tyyppi koodi;
    QString nimi;
    QIcon kuvake;
};

class TositeTyyppiModel : public QAbstractListModel
{
public:    
    TositeTyyppiModel(QObject *parent = nullptr);

    enum {
        KoodiRooli = Qt::UserRole,
        NimiRooli = Qt::DisplayRole
    };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QString nimi(int koodi) const;
    QIcon kuvake(int koodi) const;

protected:
    void lisaa(TositeTyyppi::Tyyppi koodi, const QString& nimi, const QString& kuvake);

    QList<TositeTyyppiTietue> tyypit_;
    QMap<int,TositeTyyppiTietue> map_;
};

#endif // TOSITETYYPPIMODEL_H
