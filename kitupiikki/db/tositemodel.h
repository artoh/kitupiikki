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

#ifndef TOSITEMODEL_H
#define TOSITEMODEL_H

#include <QAbstractTableModel>
#include <QDate>
#include <QSqlDatabase>
#include <QTextDocument>


#include "db/vientimodel.h"
#include "db/tositelaji.h"
#include "db/jsonkentta.h"
#include "db/liitemodel.h"
#include "db/kirjanpito.h"

#include "raportti/raportinkirjoittaja.h"

/**
 * @brief Tositteen tiedot
 */
class TositeModel : public QAbstractTableModel
{
    Q_OBJECT
public:


    enum VientiSarake
    {
        PVM, TILI, DEBET, KREDIT, KOHDENNUS, ALV, SELITE
    };


    TositeModel(QSqlDatabase *tietokanta, QObject *parent = nullptr);


    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;


    /**
     * @brief Tositteen id
     * @return -1 tarkoittaa, ettei tositetta vielä tallennettu
     */
    int id() const { return map_.value("id").toInt(); }
    QDate pvm() const { return map_.value("pvm").toDate(); }
    QString otsikko() const { return map_.value("otsikko").toString(); }
    QString kommentti() const { return map_.value("kommentti").toString(); }

    QDateTime luontiAika() const { return map_.value("luotu").toDateTime(); }
    QDateTime muokattuAika() const { return map_.value("muokattu").toDateTime(); }

    int tunniste() const { return map_.value("tunniste").toInt(); }
    Tositelaji tositelaji() const;

    /**
     * @brief Tiliotetilin numero
     *
     * Jos tämä tosite on tili, palauttaa sen tilin numeron,
     * jonka tiliote tämä on
     *
     * @return
     */
    int tiliotetili() const { return map_.value("tiliotetili").toInt(); }


    bool muokkausSallittu() const;
    VientiModel *vientiModel() { return vientiModel_; }
    LiiteModel* liiteModel() { return &liitteet_; }

    QSqlDatabase *tietokanta() { return tietokanta_; }

    JsonKentta *json() { return &json_; }

    /**
     * @brief Palauttaa seuraavan mahdollisen tunnistenumeron
     * @return
     */
    int seuraavaTunnistenumero() const;

    /**
     * @brief Kertoo, onko sanottu tunnistenumero kelvollinen
     * @param tunniste
     * @return
     */
    bool kelpaakoTunniste(int tunnistenumero) const;

    /**
     * @brief Onko tallentamattomia muokkauksia
     * @return tosi, jos muokattu
     */
    bool muokattu();

signals:
    void tositettaMuokattu(bool onko);
    void tyhjennetty();


public slots:
    void asetaPvm(const QDate& pvm);
    void asetaOtsikko(const QString& otsikko);
    void asetaKommentti(const QString& kommentti);

    void asetaTunniste(int tunniste);
    void asetaTositelaji(int tositelajiId);
    void asetaTiliotetili(int tiliNumero);

    void lataa(int id);

    /**
     * @brief Tyhjentää tositteen
     *
     * Päivämäärä ja tositelaji jäävät kuitenkin edellisestä
     */
    void tyhjaa();
    bool tallenna();
    bool poista();

    /**
     * @brief Luo uuden tositteen tämän tositteen pohjalta
     * @param pvm
     * @param otsikko
     */
    void uusiPohjalta(const QDate& pvm, const QString& otsikko);

    void lataaMapista(QVariant* variant);

public:
    /**
     * @brief Tulostettava tosite
     * @return
     */
    RaportinKirjoittaja tuloste();

    /**
     * @brief Tietokannan tiedot raakamuodossa
     * @return
     */
    RaportinKirjoittaja selvittelyTuloste();



protected:
    int id_;
    QDate pvm_;
    QString otsikko_;
    QString kommentti_;
    int tunniste_;
    int tositelaji_;
    int tiliotetili_;
    QDateTime luotu_;
    QDateTime muokattuAika_;

    JsonKentta json_;

    QSqlDatabase *tietokanta_;

    bool muokattu_;


    LiiteModel liitteet_;

    VientiModel* vientiModel_;
    LiiteModel* liiteModel_;

    QVariantMap map_;
    QVariantMap muokkaamaton_;

    QList<QVariantMap> viennit_;

};

#endif // TOSITEMODEL_H
