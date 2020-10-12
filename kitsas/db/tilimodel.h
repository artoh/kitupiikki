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

#ifndef TILIMODEL_H
#define TILIMODEL_H

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QList>
#include <QSet>

#include "db/tili.h"

/**
 * @brief Tilit
 *
 * Tilien tiedot
 *
 *
 */
class TiliModel : public QAbstractTableModel
{
    Q_OBJECT
public:

    enum Sarake
    {
        NRONIMI, NUMERO, NIMI, TYYPPI, ALV, SALDO
    };

    enum
    {
        IdRooli = Qt::UserRole + 1,
        NroRooli = Qt::UserRole + 2,
        NimiRooli = Qt::UserRole + 3,
        NroNimiRooli = Qt::UserRole + 4,
        OtsikkotasoRooli = Qt::UserRole + 5,
        TyyppiRooli = Qt::UserRole + 6,        
        TilaRooli = Qt::UserRole + 8,
        OhjeRooli = Qt::UserRole + 9,
        LajitteluRooli = Qt::UserRole + 10,
        SaldoRooli = Qt::UserRole + 12
    };


    TiliModel(QObject *parent = nullptr);
    ~TiliModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    Tili* lisaaTili(int numero, int otsikkotaso);
    void tallenna(Tili *tili);

    void poistaRivi( int riviIndeksi );

    Tili *tili(const QString& tilinumero) const;

    Tili *tili(int numero) const;
    Tili tiliIndeksilla(int i) const { return *tiliLista_.at(i); }
    Tili *tiliPIndeksilla(int i) const { return tiliLista_.at(i);}

    Tili tiliNumerolla(int numero, int otsikkotaso = 0) const;
    Tili tiliIbanilla(const QString& iban) const;

    QString nimi(int numero) const;

    /**
     * @brief Palauttaa ensimmäisen halutun tyyppisen tilin
     * @param luonne
     * @return
     */
    Tili tiliTyypilla(TiliLaji::TiliLuonne tyyppi) const;
    Tili tiliTyypilla(const QString& tyyppikoodi) const;

    [[deprecated]] QStringList laskuTilit() const;

    void lataa(QVariantList lista);

    void asetaSuosio(int tili, Tili::TiliTila tila);

    void paivitaTilat();

    double saldo(int tilinumero) { return saldot_.value(tilinumero);}

public slots:
    void haeSaldot();
    void paivitaNimet();

private slots:
    void saldotSaapuu(QVariant* saldot);    

protected:
    void tyhjenna();


protected:    
    QList<Tili*> tiliLista_;
    QHash<int,Tili*> nroHash_;
    QHash<int,QString> nimiHash_;

    QString muoto_;
    QSet<int> piilotetut_;
    QSet<int> suosikit_;
    QSet<int> naytettavat_;
    QHash<int,double> saldot_;

};

#endif // TILIMODEL_H
