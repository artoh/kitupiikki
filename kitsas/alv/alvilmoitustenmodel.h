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

#ifndef ALVILMOITUSTENMODEL_H
#define ALVILMOITUSTENMODEL_H

#include <QDate>
#include <QAbstractTableModel>
#include <QList>

class VeroVarmenneTila;
class AlvKaudet;

#include "model/euro.h"

/**
 * @brief Tehtyjen arvonlisäilmoitusten model
 */
class AlvIlmoitustenModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Sarake
    {
        ALKAA, PAATTYY, ERAPVM, VERO, TILA
    };
    enum
    {
        TositeIdRooli = Qt::UserRole,
        PaattyyRooli = Qt::UserRole + 1,
        EraPvmRooli = Qt::UserRole + 2,
        AlkaaRooli = Qt::UserRole + 3,
        MapRooli = Qt::UserRole + 4
    };

    AlvIlmoitustenModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent=QModelIndex()) const;
    int columnCount(const QModelIndex &parent=QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    qlonglong marginaalialijaama(const QDate& paiva, int kanta) const;
    bool onkoIlmoitettu(const QDate& paiva) const;

    QDate viimeinenIlmoitus() const;

    /**
     * @brief Alv-ilmoituksen eräpäivä
     * @param loppupaiva Verokauden viimeinen päivä
     * @return
     */
    QDate erapaiva(const QDate& loppupaiva) const;    

    AlvKaudet* kaudet() { return kaudet_;}

public slots:
    void lataa();
    void dataSaapuu(QVariant* data);
    void kaudetPaivitetetty();

protected:
    class AlvIlmoitusTieto {
    public:
        AlvIlmoitusTieto();
        AlvIlmoitusTieto(const QVariantMap& data);

        int tositeId() const { return tositeId_;}
        QDate alkaa() const { return alkaa_;}
        QDate paattyy() const { return paattyy_;}
        Euro maksettava() const { return maksettava_;}
        QVariantMap marginaaliAliJaama() const { return marginaaliAliJaama_;}
        QDateTime ilmoitettu() const { return ilmoitettu_;}

        QVariantMap map() const;

    protected:
        int tositeId_;
        QDate alkaa_;
        QDate paattyy_;
        Euro maksettava_;
        QVariantMap marginaaliAliJaama_;
        QDateTime ilmoitettu_;
    };

protected:
    QList<AlvIlmoitusTieto> ilmoitukset_;

    VeroVarmenneTila* varmenneTila_;
    AlvKaudet* kaudet_;

};

#endif // ALVILMOITUSTENMODEL_H
