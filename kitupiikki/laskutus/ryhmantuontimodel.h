/*
   Copyright (C) 2018 Arto Hyvättinen

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
#ifndef RYHMANTUONTIMODEL_H
#define RYHMANTUONTIMODEL_H

#include <QAbstractTableModel>

class RyhmanTuontiModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake {
        EITUODA, NIMI, LAHIOSOITE, POSTIOSOITE, POSTINUMERO, OSOITE, SAHKOPOSTI, YTUNNUS
    };

    RyhmanTuontiModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    static QString otsikkoTeksti(int sarakeEnum);

    void lataaCsv(const QString& tiedostonNimi);
    bool onkoOtsikkoRivi() const { return otsikkorivi_;}
    void asetaOtsikkoRivi(bool onko);

    int muoto(int sarake) const { return  sarakkeet_.at(sarake);}
    void asetaMuoto(int sarake, int muoto);

protected:
    void arvaaSarakkeet();

    QList<QStringList> csv_;
    QList<int> sarakkeet_;
    bool otsikkorivi_ = false;

};

#endif // RYHMANTUONTIMODEL_H
