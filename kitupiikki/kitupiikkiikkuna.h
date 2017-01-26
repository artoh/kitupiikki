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

#ifndef KITUPIIKKIIKKUNA_H
#define KITUPIIKKIIKKUNA_H

#include <QMainWindow>
#include <QMap>
#include <QStack>

class QStackedWidget;
class QAction;
class QActionGroup;
class QToolBar;
class AloitusSivu;
class KirjausSivu;
class SelausWg;
class MaaritysSivu;
class QDateEdit;
class QDockWidget;

#include "db/tilikausi.h"

/**
 * @brief Ohjelmiston pääikkuna
 *
 * Pääikkunassa on vasemmalla työkalurivi, josta toimintosivu valitaan
 *
 */
class KitupiikkiIkkuna : public QMainWindow
{
    Q_OBJECT
public:
    KitupiikkiIkkuna(QWidget *parent = 0);
    ~KitupiikkiIkkuna();

    enum Sivu { ALOITUSSIVU, KIRJAUSSIVU, SELAUSSIVU, TULOSTESIVU, MAARITYSSIVU, OHJESIVU    };

signals:

public slots:
    /**
     * @brief Vaihtaa näytettävän sivun
     * @param mikasivu Mikä sivu näytetään (enum)
     */
    void valitseSivu(int mikasivu, bool paluu = false);

    /**
     * @brief Tehdään toiminto. Tätä käytetään aloitussivun nappien yhdistämisessä.
     * @param toiminto Toiminnon nimi
     */
    void toiminto(const QString& toiminto);

    void kirjanpitoLadattu();

    void palaaSivulta();

    void selaaTilia(int tilinumero, Tilikausi tilikausi);


protected slots:
    void aktivoiSivu(QAction* aktio);
    void naytaTosite(int tositeid);


protected:
    void mousePressEvent(QMouseEvent *event);

protected:

    QStackedWidget *pino;
    QToolBar *toolbar;
    QActionGroup *aktioryhma;
    QAction *sivuaktiot[7];

    QDockWidget *harjoitusDock;

    AloitusSivu *aloitussivu;
    KirjausSivu *kirjaussivu;
    SelausWg *selaussivu;
    MaaritysSivu *maarityssivu;
    QStack<int> edellisetIndeksit;

protected:
    QAction *luosivuAktio(const QString& nimi, const QString& kuva,
                          const QString& vihje, const QString& pikanappain,
                          Sivu sivut);
    void luoPalkkiJaSivuAktiot();

    void luoHarjoitusDock();


};

#endif // KITUPIIKKIIKKUNA_H
