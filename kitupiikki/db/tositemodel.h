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

#include <QObject>
#include <QDate>
#include <QSqlDatabase>
#include <QTextDocument>

#include "db/vientimodel.h"
#include "db/tositelaji.h"
#include "db/jsonkentta.h"
#include "db/liitemodel.h"
#include "db/kirjanpito.h"

/**
 * @brief Tositteen tiedot
 */
class TositeModel : public QObject
{
    Q_OBJECT
public:
    TositeModel(QSqlDatabase *tietokanta, QObject *parent = 0);

    /**
     * @brief Tositteen id
     * @return -1 tarkoittaa, ettei tositetta vielä tallennettu
     */
    int id() const { return id_; }
    QDate pvm() const { return pvm_; }
    QString otsikko() const { return otsikko_; }
    QString kommentti() const { return kommentti_; }

    int tunniste() const { return tunniste_; }
    Tositelaji tositelaji() const;

    /**
     * @brief Tiliotetilin numero
     *
     * Jos tämä tosite on tili, palauttaa sen tilin numeron,
     * jonka tiliote tämä on
     *
     * @return
     */
    int tiliotetili() const { return tiliotetili_; }


    bool muokkausSallittu() const;
    VientiModel *vientiModel() { return vientiModel_; }
    LiiteModel* liiteModel() { return liiteModel_; }

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
    void asetaTiliotetili(int tiliId);


    void lataa(int id);

    /**
     * @brief Tyhjentää tositteen
     *
     * Päivämäärä ja tositelaji jäävät kuitenkin edellisestä
     */
    void tyhjaa();
    bool tallenna();
    bool poista();

public:
    /**
     * @brief Palauttaa tositteen html-muodossa tulostukseen
     * @return
     */
    QString html();



protected:
    int id_;
    QDate pvm_;
    QString otsikko_;
    QString kommentti_;
    int tunniste_;
    int tositelaji_;
    int tiliotetili_;

    JsonKentta json_;

    QSqlDatabase *tietokanta_;

    bool muokattu_;


    VientiModel* vientiModel_;
    LiiteModel* liiteModel_;
};

#endif // TOSITEMODEL_H
