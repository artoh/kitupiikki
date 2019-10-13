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
        YsiRooli = Qt::UserRole + 7,
        TilaRooli = Qt::UserRole + 8,
        OhjeRooli = Qt::UserRole + 9,
        MaksutapaRooli = Qt::UserRole + 10
    };


    TiliModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;

    void lisaaTili(const Tili &uusi);
    void poistaRivi( int riviIndeksi );

    [[deprecated]] Tili tiliIdllaVanha(int id) const;

    Tili *tili(const QString& tilinumero) const;

    Tili *tiliPNumerolla(int numero) const;
    Tili tiliIndeksilla(int i) const { return *tiliLista_.at(i); }
    Tili *tiliPIndeksilla(int i) const { return tiliLista_.at(i);}

    Tili tiliNumerolla(int numero, int otsikkotaso = 0) const;
    [[deprecated]] Tili tiliYsiluvulla(int ysiluku) const;
    Tili tiliIbanilla(const QString& iban) const;
    /**
     * @brief Palauttaa tilin, jolle kirjataan edellisiltä tilikausilta kertynyt yli/alijäämä
     * @return
     */
    Tili edellistenYlijaamaTili() const;

    /**
     * @brief Palauttaa ensimmäisen halutun tyyppisen tilin
     * @param luonne
     * @return
     */
    Tili tiliTyypilla(TiliLaji::TiliLuonne tyyppi) const;

    QStringList laskuTilit() const;

    /**
     * @brief Palauttaa annettua palkkatili-avainta vastaavan tilin numeron
     * @param avain
     * @return
     */
    int palkankirjaustili(const QString& avain) const { return palkkatilit_.value(avain); }

    [[deprecated]] JsonKentta *jsonIndeksilla(int i);

    bool onkoMuokattu() const;

    [[deprecated]] void lataa();
    void lataa(QVariantList lista);
    [[deprecated]] bool tallenna(bool tietokantaaLuodaan = false);

public slots:
    void haeSaldot();

private slots:
    void saldotSaapuu(QVariant* saldot);

protected:
    void tyhjenna();
    void paivitaTilat();

protected:    

    QList<Tili> tilit_;
    QList<int> poistetutIdt_;

    QList<Tili*> tiliLista_;
    QHash<int,Tili*> nroHash_;
    QMap<QString,int> palkkatilit_;

    int laajuus_ = 2;
    QString muoto_;
    QSet<int> piilotetut_;
    QSet<int> suosikit_;
    QHash<int,double> saldot_;

};

#endif // TILIMODEL_H
