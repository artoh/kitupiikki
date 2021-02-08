/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef TILINAVAUSMODEL_H
#define TILINAVAUSMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QMap>

#include "db/kirjanpito.h"
#include "model/tosite.h"

/**
 * @brief Tilinavauksessa oleva yksi erä
 */
class AvausEra {
public:
    AvausEra(qlonglong saldo = 0l, const QString& eranimi=QString(), int kohdennus=0, int vienti=0,
             int kumppaniId=0, QString kumppaniNimi=QString());

    QString eranimi() const { return eranimi_; }
    int kohdennus() const { return kohdennus_;}
    qlonglong saldo() const { return saldo_; }
    int vienti() const { return vienti_;}
    int kumppaniId() const { return kumppaniId_;}
    QString kumppaniNimi() const { return  kumppaniNimi_;}

    void asetaNimi(const QString& nimi) { eranimi_ = nimi;}
    void asetaKohdennus(int kohdennus) { kohdennus_ = kohdennus; }
    void asetaSaldo(qlonglong saldo) { saldo_= saldo;}
    void asetaKumppani(const QVariantMap& map);

protected:
    QString eranimi_;
    int kohdennus_ = 0;
    qlonglong saldo_ = 0l;
    int vienti_ = 0;
    int kumppaniId_ = 0;
    QString kumppaniNimi_;
};

/**
 * @brief Tilinavaukset
 *
 * Tätä modelia käytetään tilinavausten syöttämiseen määrittelynäkymässä
 *
 */
class TilinavausModel : public QAbstractTableModel
{
    Q_OBJECT


public:
    enum Sarake
    {
        NRO, NIMI, SALDO, ERITTELY
    };

    enum
    {
        KaytossaRooli = Qt::UserRole + 1,
        NumeroRooli = Qt::UserRole + 2,
        NimiRooli = Qt::UserRole + 3,
        ErittelyRooli = Qt::UserRole + 4,
        LajitteluRooli = Qt::UserRole + 11
    };

    enum Erittely {
        EI_ERITTELYA,
        TASEERAT,
        KOHDENNUKSET
    };

    TilinavausModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    bool onkoMuokattu() const { return muokattu_; }

    void asetaErat(int tili, QList<AvausEra> erat);
    QList<AvausEra> erat(int tili) const;

public slots:
    bool tallenna();
    void lataa();

    void paivitaInfo();

    void ladattu();

protected:
    void idTietoSaapuu(QVariant* data);

    static qlonglong erasumma(const QList<AvausEra>& erat);


signals:
    void tilasto(qlonglong vastaava, qlonglong vastattava, qlonglong tulos);

protected:
    QMap<int, QList<AvausEra>> erat_;
    Tosite *tosite_;

    bool muokattu_;
    int kaudenTulosIndeksi_ = 0;
    bool alvVelkaVaroitus = false;
    bool alvSaatavaVaroitus = false;
};

#endif // TILINAVAUSMODEL_H
