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

#ifndef TOSITESELAUSMODEL_H
#define TOSITESELAUSMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include <QList>
#include <QSet>
#include <QSqlQuery>

#include "model/euro.h"

class SQLiteModel;

/**
 * @brief Yhden tositteen tiedot tositteiden selauksessa
 */
class TositeSelausRivi
{
public:
    TositeSelausRivi(const QVariantMap& data, bool samakausi = false);
    TositeSelausRivi(QSqlQuery& data, bool samakausi=false);
    QVariant data(int sarake, int role, int selaustila) const;

    QString getSarja() const { return sarja; }
    int getTyyppi() const { return tositeTyyppi; }
    QString getEtsi() const { return etsiTeksti;}
    bool getHuomio() const { return huomio;}

protected:

    int tositeId;
    int tila;
    QDate pvm;
    int tositeTyyppi;
    QString tositeTunniste;
    QString vertailuTunniste;

    QString otsikko;
    QString kumppani;
    Euro summa;

    bool liitteita;
    QString etsiTeksti;
    QString sarja;

    bool huomio;
};

/**
 * @brief Tositteiden selauksen model
 */
class TositeSelausModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        TUNNISTE, PVM, TOSITETYYPPI, SUMMA, ASIAKASTOIMITTAJA, OTSIKKO
    };

    enum
    {
        TositeIdRooli = Qt::UserRole,
        TositeTyyppiRooli = Qt::UserRole + 1,
        TositeSarjaRooli = Qt::UserRole + 6,
        EtsiRooli = Qt::UserRole + 128

    };

    enum {
        SAAPUNEET = 1,
        LUONNOKSET = 2,
        KIRJANPIDOSSA = 3,
        POISTETUT = 4
    };

    TositeSelausModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    QList<int> tyyppiLista() const;
    QStringList sarjaLista() const;

    int tyyppi(int rivi) const { return rivit_.at(rivi).getTyyppi();}
    QString sarja(int rivi) const { return rivit_.at(rivi).getSarja();}
    QString etsiTeksti(int rivi) const { return rivit_.at(rivi).getEtsi();}
    bool huomio(int rivi) const { return rivit_.at(rivi).getHuomio();}

public slots:
    void lataa(const QDate& alkaa, const QDate& loppuu, int tila = KIRJANPIDOSSA);
    void tietoSaapuu(QVariant *var);
    void latausVirhe();

protected:
    void lataaSqlite(SQLiteModel* sqlite, const QDate& alkaa, const QDate& loppuu);

    QList<TositeSelausRivi> rivit_;

    QSet<int> kaytetytTyypit_;
    QSet<QString> kaytetytSarjat_;

    int tila_ = KIRJANPIDOSSA;
    bool samakausi_ = false;
    bool ladataan_ = false;

};

#endif // TOSITESELAUSMODEL_H
