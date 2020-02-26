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
#ifndef LASKUTAULUMODEL_H
#define LASKUTAULUMODEL_H

#include <QAbstractTableModel>
#include <QDate>

class LaskuTauluModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum LaskuValinta { LUONNOS, LAHETETTAVA, KAIKKI, AVOIMET, ERAANTYNEET };
    enum LaskuSarake { NUMERO, PVM, ERAPVM, SUMMA, LAHETYSTAPA, MAKSAMATTA, ASIAKASTOIMITTAJA, OTSIKKO };


    enum { AvoinnaRooli = Qt::UserRole + 1,
         EraIdRooli = Qt::UserRole + 2,
         LaskuPvmRooli = Qt::UserRole + 3,
         AsiakasToimittajaNimiRooli = Qt::UserRole + 4,
         TiliRooli = Qt::UserRole + 5,
         ViiteRooli = Qt::UserRole + 6,
         AsiakasToimittajaIdRooli = Qt::UserRole + 8,
         TositeIdRooli = Qt::UserRole + 10,
         TyyppiRooli = Qt::UserRole + 11,
         TunnisteRooli = Qt::UserRole + 12,
         SarjaRooli = Qt::UserRole + 13,
         EraMapRooli = Qt::UserRole + 14,
         NumeroRooli = Qt::UserRole + 15,
         EraPvmRooli = Qt::UserRole + 16,
         LaskutustapaRooli = Qt::UserRole + 17,
         SeliteRooli = Qt::UserRole + 18,
         TilaRooli = Qt::UserRole + 19};

    explicit LaskuTauluModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void lataaAvoimet(bool ostoja = false);
    void paivita(bool ostoja = false, int valinta = KAIKKI, QDate mista=QDate(), QDate mihin = QDate());
    void paivitaNakyma();

private slots:
    void tietoSaapuu(QVariant* var);

protected:
    QVariantList lista_;
    bool ostoja_ = false;
    int valinta_;
    QDate mista_;
    QDate mihin_;

};

#endif // LASKUTAULUMODEL_H
