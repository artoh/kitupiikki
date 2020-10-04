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
#ifndef REKISTERITUONTIMODEL_H
#define REKISTERITUONTIMODEL_H

#include <QAbstractTableModel>

class RekisteriTuontiModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Sarakkeet {
        EITUODA,
        NIMI,
        LAHIOSOITE,
        POSTINUMERO,
        KAUPUNKI,
        POSTIOSOITE,
        SAHKOPOSTI,
        PUHELIN,
        YTUNNUS,
        VERKKOLASKUOSOITE,
        VERKKOLASKUVALITTAJA,
        MAA,
        KIELI,
        LISATIETO
    };

    enum {
        MALLI,
        MUOTO
    };

    explicit RekisteriTuontiModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    static QString otsikkoTeksti(int sarake);

    int lataaCsv(const QString& tiedostonnimi);

    virtual QVariantList lista() const;

signals:
    void otsikkorivit(bool onko);
    void tuotu();

public slots:
    void asetaOtsikkorivi(bool otsikkorivi);

protected:
    virtual void arvaaSarakkeet();
    virtual QString otsikkoTekstini(int sarake) const;
    

    QList<QStringList> csv_;
    QVector<int> sarakkeet_;
    bool otsikkorivi_ = false;
};

#endif // REKISTERITUONTIMODEL_H
