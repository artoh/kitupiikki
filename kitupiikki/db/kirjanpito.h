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


/**
 * @dir db
 * @brief Tietokantaan (kirjanpidon hakemistoon) liittyvät luokat
 *
 */

#ifndef KIRJANPITO_H
#define KIRJANPITO_H

#include <QObject>
#include <QMap>
#include <QDir>
#include <QSqlDatabase>
#include <QDate>

#include "tili.h"
#include "tilikausi.h"
#include "tositetyyppi.h"

/**
 * @brief Kirjanpidon käsittely
 *
 * Ainokainen, jonka kautta käsitellään tietokantaa
 *
 *
 */
class Kirjanpito : public QObject
{
    Q_OBJECT

protected:
    Kirjanpito(QObject *parent = 0);
public:
    ~Kirjanpito();

    QString asetus(const QString& avain) const;
    void aseta(const QString& avain, const QString& arvo);

    /**
     * @brief Hakemisto, jossa kirjanpito (kitupiikki.sqlite)
     * @return hakemisto
     */
    QDir hakemisto();

    /**
     * @brief Luettelo viimeksi avatuista tiedostoista
     * @return Luettelo, rivit muotoa polku;otsikko
     */
    QStringList viimeisetTiedostot() const;

    /**
     * @brief Käytetäänkö harjoittelutilassa
     * @return tosi, jos harjoitellaan
     */
    bool onkoHarjoitus() const { return asetus("harjoitus").toInt() > 0 ; }

    /**
     * @brief Nykyinen tai harjoittelutilassa muokattu päivämäärä
     *
     * Harjoittelutila mahdollistaa päivämäärän muuttamisen, jotta päästään testaamaan
     * tilinpäätökseen tms. liittyviä toimia
     * @return Päivämäärä
     */
    QDate paivamaara() const;

    /**
     * @brief Päivämäärä, johon saakka tilit on päätetty eli ei voi enää muokata
     * @return
     */
    QDate tilitpaatetty() const { return tilitpaatettupvm; }
    QDate viimeinenpaiva() const { return tilikaudet_.last().paattyy(); }

    Tili tili(int tilinumero) const { return tilit_[tilinumero] ; }
    QList<Tili> tilit(QString tyyppisuodatin = QString(), int tilasuodatin = 0) const;

    QList<Tilikausi> tilikaudet() const { return tilikaudet_; }
    QList<TositeTyyppi> tositelajit() const { return tositetyypit_; }

    Tilikausi tilikausiPaivalle(const QDate &paiva) const;

signals:
    void tietokantaVaihtui();
    void kirjanpitoaMuokattu();
    void palaaEdelliselleSivulle();

public slots:
    /**
     * @brief Avaa kirjanpitotietokannan
     * @param tiedosto Kirjanpidon kitupiikki.sqlite-tiedoston täydellinen polku
     * @return tosi, jos onnistuu
     */
    bool avaaTietokanta(const QString& tiedosto);

    /**
     * @brief Lataa tietokannan uudelleen rakenteen muutoksen jälkeen
     * @return tosi, jos onnistui
     */
    bool lataaUudelleen();

    void asetaHarjoitteluPvm(const QDate& pvm);

    void muokattu();

protected:
    QMap<QString,QString> asetukset;
    QMap<int,Tili> tilit_;
    QList<Tilikausi> tilikaudet_;
    QList<TositeTyyppi> tositetyypit_;

    QString polkuTiedostoon;
    QSqlDatabase tietokanta;
    QMap<QString,QString> viimetiedostot;
    QDate harjoitusPvm;
    QDate tilitpaatettupvm;

public:
    /**
     * @brief Staattinen funktio, jonka kautta Kirjanpitoon päästään käsiksi
     * @return
     */
    static Kirjanpito *db();

private:
    static Kirjanpito *instanssi__;
};

#endif // KIRJANPITO_H
