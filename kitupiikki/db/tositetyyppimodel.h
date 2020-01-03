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
        KULULASKU = 120,
        TULO = 200,
        MYYNTILASKU = 210,
        HYVITYSLASKU = 214,
        MAKSUMUISTUTUS = 216,
        SIIRTO = 300,
        TILIOTE = 400,
        PALKKA = 500,
        MUISTIO = 700,
        LIITETIETO = 800,
        JARJESTELMATOSITE = 1000,
        TILINAVAUS = 9010,
        ALVLASKELMA = 9100,
        POISTOLASKELMA = 9910,
        JAKSOTUS = 9920,
        TULOVERO = 9930
    };    
}


struct TositeTyyppiTietue
{
    TositeTyyppiTietue() {;}
    TositeTyyppiTietue(TositeTyyppi::Tyyppi uKoodi, const QString& uNimi, const QString& uKuvake = QString(), bool uLisattavissa = false);

    TositeTyyppi::Tyyppi koodi;
    QString nimi;
    QIcon kuvake;
    bool lisattavissa;
};

class TositeTyyppiModel : public QAbstractListModel
{
    Q_OBJECT
public:    
    TositeTyyppiModel(QObject *parent = nullptr);

    enum {
        KoodiRooli = Qt::UserRole,
        NimiRooli = Qt::DisplayRole,
        LisattavissaRooli = Qt::UserRole + 1
    };

    enum Sarjavalinta {
        SAMAANSARJAAN = 0,
        TOSITELAJIT = 1,
        KATEISSARJA = 2
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QString nimi(int koodi) const;
    QIcon kuvake(int koodi) const;
    bool onkolisattavissa(int koodi) const;
    QString sarja(int koodi, bool kateinen = false) const;

protected:
    void lisaa(TositeTyyppi::Tyyppi koodi, const QString& nimi, const QString& kuvake, bool lisattavissa = true);

    QList<TositeTyyppiTietue> tyypit_;
    QMap<int,TositeTyyppiTietue> map_;
};

#endif // TOSITETYYPPIMODEL_H
