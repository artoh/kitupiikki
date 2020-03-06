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

/**
 * @brief Yhden tositteen tiedot tositteiden selauksessa
 */
struct TositeSelausRivi
{
    int tositeId;
    QDate pvm;
    int tositeLaji;
    int tositeTunniste;

    QString otsikko;
    qlonglong summa;

    bool liitteita;

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
    };

    enum {
        SAAPUNEET = 1,
        LUONNOKSET = 2,
        KIRJANPIDOSSA = 3
    };

    TositeSelausModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    QList<int> tyyppiLista() const;

    QStringList lajiLista() const { return kaytetytLajinimet; }

public slots:
    void lataa(const QDate& alkaa, const QDate& loppuu, int tila = KIRJANPIDOSSA);
    void tietoSaapuu(QVariant *var);

protected:
    QList<TositeSelausRivi> rivit;
    QStringList kaytetytLajinimet;

    QVariantList lista_;
    QSet<int> kaytetytTyypit_;

    int tila_ = KIRJANPIDOSSA;
    bool samakausi_ = false;

};

#endif // TOSITESELAUSMODEL_H
