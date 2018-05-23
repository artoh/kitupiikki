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

#ifndef ALVILMOITUSDIALOG_H
#define ALVILMOITUSDIALOG_H

#include <QDialog>
#include "raportti/raportinkirjoittaja.h"

namespace Ui {
class AlvIlmoitusDialog;
}

/**
 * @brief Arvonlisäilmoituksen tekemisen dialogi
 *
 * Näyttäää esikatselun valitun kauden alv-ilmoituksesta ja tekee alv-kirjaukset jos dialogi hyväksytään.
 *
 * Käytetään AlvIlmoitusDialogi::teeAlvIlmoitus() -funktiolla
 */
class AlvIlmoitusDialog : public QDialog
{
    Q_OBJECT

protected:
    explicit AlvIlmoitusDialog(QWidget *parent = 0);
    ~AlvIlmoitusDialog();

public:
    /**
     * @brief Näyttää ilmoitusdialogin ja jos se hyväksytään, tekee kirjaukset
     * @param alkupvm Ilmoituskauden alkupäivä
     * @param loppupvm Ilmoituskauden loppupäivä
     * @return pvm johon asti alv ilmoitettu, tai Null jos ilmoitus perutaan
     */
    static QDate teeAlvIlmoitus(QDate alkupvm, QDate loppupvm);

private:
    /**
     * @brief Varsinainen alv-ilmoituksen teko
     * @param alkupvm
     * @param loppupvm
     * @return
     */
    bool alvIlmoitus(QDate alkupvm, QDate loppupvm);

    Ui::AlvIlmoitusDialog *ui;
    RaportinKirjoittaja *kirjoittaja;

    /**
     * @brief Kirjoittaa raporttiin väliotsikon
     * @param teksti
     */
    void otsikko(const QString& teksti);
    /**
     * @brief Kirjoittaa raporttiin tietorivin
     * @param nimike
     * @param senttia
     */
    void luku(const QString& nimike, int senttia, bool viiva = false);

    /**
     * @brief Tekee erääntyneen maksuperusteisen alvin tilitykset
     * @param paivayksesta Päiväys, jota vanhemmat erääntyneitä
     * @return false, jos peruttiin
     */
    bool maksuperusteisenTilitys(const QDate& paivayksesta, const QDate &tilityspvm);

};

#endif // ALVILMOITUSDIALOG_H
