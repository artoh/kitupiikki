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

#ifndef MAARITYSSIVU_H
#define MAARITYSSIVU_H

#include <QWidget>
#include <QIcon>

#include "maarityswidget.h"
#include "kitupiikkisivu.h"

class QStackedWidget;
class QListWidget;
class QVBoxLayout;
class QListWidgetItem;
class QButton;
class QPushButton;

/**
 * @brief Määrityssivun sisältävä QWidget
 */
class MaaritysSivu : public KitupiikkiSivu
{
    Q_OBJECT
public:

    enum Sivut
    {
        PERUSVALINNAT,
        TILIKARTTA,
        TOSITELAJIT,
        KOHDENNUS,
        TILINAVAUS,
        ALV,
        LASKUTUS,
        SAHKOPOSTI,
        RAPORTIT,
        LIITETIETOKAAVA

    };


    MaaritysSivu();

    void siirrySivulle();
    bool poistuSivulta();

public slots:
    /**
     * @brief Peru-napilla nollataan lomake
     */
    void peru();
    /**
     * @brief Tallenna-napilla tallennetaan
     */
    void tallenna();

    /**
     * @brief Valitaan näytettävä sivu
     * @param item Sivut-enum
     */
    void valitseSivu( QListWidgetItem *item);
    /**
     * @brief Valitaan näytettävä sivu
     * @param sivu
     */
    void valitseSivu(QString otsikko);

    /**
     * @brief Päivittää määritysten mukaan sen, mitkä sivut näkyvät
     */
    void paivitaNakyvat();

    /**
     * @brief Käynnistää tilikartan vientivelhon
     */
    void vieTilikartta();

protected:
    /**
     * @brief Lisää sivun luetteloon
     * @param otsikko Luettelossa oleva teksti
     * @param sivu Sivun enum, jonka perusteella valittu sivu luodaan
     * @param kuvake Kuvake
     * @param tallennaPeruNapit Näytetäänkö sivulla Tallenna- ja peru-napit
     */
    void lisaaSivu(const QString& otsikko, Sivut sivu,
                   const QIcon& kuvake = QIcon());

protected:
    QListWidget *lista;

    MaaritysWidget *nykyinen;
    QListWidgetItem *nykyItem;
    QVBoxLayout *sivuleiska;

    QPushButton *vienappi;
    QPushButton *tallennanappi;
    QPushButton *perunappi;

};

#endif // MAARITYSSIVU_H
