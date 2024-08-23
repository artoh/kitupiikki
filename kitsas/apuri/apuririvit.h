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
#ifndef APURIRIVIT_H
#define APURIRIVIT_H

#include <QAbstractTableModel>
#include "model/tositevienti.h"

#include "apuririvi.h"
class Tili;
class Tosite;

class ApuriRivit : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ApuriRivit(QObject *parent = nullptr);

    enum Sarakkeet {
        TILI, ALV, EUROA
    };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lisaa(const QVariantMap& map);

    int lisaaRivi(int tili=0, const QDate& pvm = QDate());
    void lisaaRivi(const ApuriRivi& rivi);
    void poistaRivi(int rivi);
    
    ApuriRivi* rivi(int indeksi);
    ApuriRivi at(int indeksi) const;
    ApuriRivi eka() const;

    void clear();

    QVariantList viennit(Tosite *tosite);

    void asetaTyyppi(TositeVienti::VientiTyyppi tyyppi, bool plusOnKredit = true);
    void asetaRivit(const QList<ApuriRivi>& rivit);

    Euro summa() const;
    QList<ApuriRivi> rivit() const;

    TositeVienti::VientiTyyppi tyyppi() const { return tyyppi_;}
    bool plusOnKredit() const { return plusOnKredit_; }

private:
    TositeVienti::VientiTyyppi tyyppi_ = TositeVienti::VientiTyyppi::TUNTEMATON;
    bool plusOnKredit_ = true;  // Näin tulolla!

    QList<ApuriRivi> rivit_;
};

#endif // APURIRIVIT_H
