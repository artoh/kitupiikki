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

class QAction;
class QActionGroup;
class QToolBar;

class AloitusSivu;

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

    enum Sivu { ALOITUSSIVU, KIRJAUSSIVU, PAIVAKIRJASIVU, PAAKIRJASIVU, TULOSTESIVU, MAARITYSSIVU, OHJESIVU    };

signals:

public slots:
    /**
     * @brief Vaihtaa näytettävän sivun
     * @param mikasivu Mikä sivu näytetään (enum)
     */
    void valitseSivu(int mikasivu);

    /**
     * @brief Tehdään toiminto. Tätä käytetään aloitussivun nappien yhdistämisessä.
     * @param toiminto Toiminnon nimi
     */
    void toiminto(const QString& toiminto);

protected slots:
    void aktivoiSivu(QAction* aktio);

protected:
    QToolBar *toolbar;
    QActionGroup *aktioryhma;

    AloitusSivu *aloitussivu;
    QAction *sivuaktiot[7];

protected:
    QAction *luosivuAktio(const QString& nimi, const QString& kuva,
                          const QString& vihje, const QString& pikanappain,
                          Sivu sivut);
    void luoPalkkiJaSivuAktiot();

};

#endif // KITUPIIKKIIKKUNA_H
