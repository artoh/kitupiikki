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
#ifndef TILIOTEMODEL_H
#define TILIOTEMODEL_H

#include <QAbstractTableModel>

#include <QDate>

class TilioteModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    struct Tilioterivi {

        QDate pvm;
        double euro = 0;
        int tili = 0;
        int kohdennus = 0;
        QString selite;
        QVariantList merkkaukset;
        int saajamaksajaId;
        QString saajamaksaja;
        QString arkistotunnus;
        QString tilinumero;
        QString viite;
        QVariantMap era;
        QDate laskupvm;
        bool harmaa = false;
        QDate jaksoalkaa;
        QDate jaksoloppuu;
        QVariantList alkuperaisetViennit;
    };

public:
    explicit TilioteModel(QObject *parent = nullptr);

    enum Sarakkeet {
        PVM, SAAJAMAKSAJA, SELITE, TILI, KOHDENNUS, EURO
    };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void lisaaRivi( const Tilioterivi& rivi);
    void poistaRivi( int rivi);
    void muokkaaRivi( int rivi, const Tilioterivi& data);

    Tilioterivi rivi(int rivi) const { return rivit_.at(rivi); }

    QVariantList viennit(int tilinumero) const;
    void lataa(QVariantList lista);

    void tuo(const QVariantList tuotavat);

public slots:
    void lataaHarmaat(int tili, const QDate& mista, const QDate& mihin);

protected slots:
    void harmaatSaapuu(QVariant* data);

protected:
    void teeTuonti();
    void siivoa(int harmaarivi, int myohemmat);

private:

    QList<Tilioterivi> rivit_;
    QVariantList tuotavat_;
};

#endif // TILIOTEMODEL_H
