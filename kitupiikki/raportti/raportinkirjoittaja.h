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

#ifndef RAPORTINKIRJOITTAJA_H
#define RAPORTINKIRJOITTAJA_H

#include <QObject>
#include <QString>
#include <QList>
#include <QPrinter>

#include "raporttirivi.h"

struct RaporttiSarake
{
    QString leveysteksti;
    int jakotekija = 0;
    int leveysprossa = 0;

};

/**
 * @brief Raporttien kirjoittaja
 *
 * Tällä luokalla kirjoitetaan raportit
 */
class RaportinKirjoittaja : public QObject
{
    Q_OBJECT
public:
    explicit RaportinKirjoittaja(QObject *parent = 0);

    void asetaOtsikko(const QString& otsikko);
    void asetaKausiteksti(const QString& kausiteksti);

    /**
     * @brief Lisää sarakkeen
     * @param leveysteksti Teksti, jolle sarake mitoitetaan
     */
    void lisaaSarake(const QString& leveysteksti);
    /**
     * @brief Lisää sarakkeen
     * @param leveysprosentti Sarakkeen leveys on % sivun leveydestä
     */
    void lisaaSarake(int leveysprosentti);

    /**
     * @brief Lisää venyvän sarakkeen
     * @param tekija Missä suhteessa jäljellä oleva tila jaetaan
     */
    void lisaaVenyvaSarake(int tekija = 100);

    /**
     * @brief Lisää euromääräisen sarakkeen
     */
    void lisaaEurosarake();
    /**
     * @brief Lisää sarakkeen päivämäärälle
     */
    void lisaaPvmSarake();

    void lisaaOtsake( RaporttiRivi otsikkorivi);
    void lisaaRivi( RaporttiRivi rivi);

    /**
     * @brief Tulostaa kirjoitetun raportin
     * @param printer
     */
    void tulosta(QPrinter *printer);


signals:

public slots:

protected:
    void tulostaYlatunniste(QPainter *painter, int sivu);


protected:
    QString otsikko_;
    QString kausiteksti_;

    QList<RaporttiSarake> sarakkeet_;
    QList<RaporttiRivi> otsakkeet_;
    QList<RaporttiRivi> rivit_;

};

#endif // RAPORTINKIRJOITTAJA_H
