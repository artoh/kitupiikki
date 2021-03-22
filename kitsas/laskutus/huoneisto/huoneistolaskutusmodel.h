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
#ifndef HUONEISTOLASKUTUSMODEL_H
#define HUONEISTOLASKUTUSMODEL_H

#include <QAbstractTableModel>

class KitsasInterface;
class YksikkoModel;

class HuoneistoLaskutusModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum { NIMI, MAARA, YKSIKKO};

    explicit HuoneistoLaskutusModel(KitsasInterface* kitsas, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void lataa(const QVariantList& lista);
    QVariantList list() const;

    void lisaaTuote(int tuote);
    void poistaRivi(int rivi);

private:
    class HuoneistoLaskutettava {
    public:
        HuoneistoLaskutettava();
        HuoneistoLaskutettava(int tuoteId, const QString& lkm);
        HuoneistoLaskutettava(const QVariantMap& map);

        int tuoteId() const { return tuoteId_;}
        QString lkm() const { return lkm_;}
        void setLkm(const QString& lkm) { lkm_ = lkm;}

        QVariantMap map() const;

    private:
        int tuoteId_;
        QString lkm_;
    };

    QList<HuoneistoLaskutettava> laskutettavat_;
    KitsasInterface* kitsas_;
    YksikkoModel* yksikot_;
};

#endif // HUONEISTOLASKUTUSMODEL_H
