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

#ifndef KIRJAUSWG_H
#define KIRJAUSWG_H

#include <QWidget>

#include "ui_kirjaus.h"
#include "tositewg.h"

#include "db/tositemodel.h"



class Kirjanpito;

class KirjausWg : public QWidget
{
    Q_OBJECT
public:
    KirjausWg(TositeModel *tositeModel);
    ~KirjausWg();

    QDate tositePvm() const;

    enum Valilehdet { VIENNIT, KOMMENTIT, LIITTEET, TILIOTE, AVUSTAJA } ;


public slots:
    void lisaaRivi();
    void tyhjenna();
    void tallenna();
    void hylkaa();

    void naytaSummat();
    void lataaTosite(int id);

    void paivitaKommenttiMerkki();
    /**
     * @brief Jos tunniste ei ole kelpo, värjätään se punaiseksi!
     */
    void paivitaTunnisteVari();

    void lisaaLiite();

    /**
     * @brief Siirtää lomakkeen tiedot modeliin
     */
    void tiedotModeliin();
    /**
     * @brief Hakee tiedot modelista lomakkeeseen;
     */
    void tiedotModelista();


    /**
     * @brief Määrää saako tositetta muokata
     *
     * Järjestelmätositteiden sekä päätettyjen tilikausien tositteiden muokkaamista ei sallita
     *
     * @param sallitaanko
     */
    void salliMuokkaus(bool sallitaanko);

    void vaihdaTositeTyyppi();


protected:
    Ui::KirjausWg *ui;
    TositeModel *model;




};

#endif // KIRJAUSWG_H
