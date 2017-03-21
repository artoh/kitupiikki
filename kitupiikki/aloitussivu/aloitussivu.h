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

#ifndef ALOITUSSIVU_H
#define ALOITUSSIVU_H

#include <QWebEngineView>
#include <QListWidget>

#include <QTextBrowser>

#include "db/tilikausi.h"
#include "kitupiikkisivu.h"

class Sisalto;



/**
 * @brief Webipohjaiset aloitus- ja ohjesivut
 *
 *
 */
class AloitusSivu : public KitupiikkiSivu
{
    Q_OBJECT

public:
    AloitusSivu();
    ~AloitusSivu();

public slots:

    void siirrySivulle();

    /**
     * @brief Sisältö kutsuu tätä, kun pyydetään selausta selaa:-protokollalla
     * @param tilinumero
     */
    void selaaTilia(int tilinumero);

    void uusiTietokanta();
    void avaaTietokanta();
    void viimeisinTietokanta(QListWidgetItem* item);

    void abouttiarallaa();

signals:
    void selaus(int tilinumero, Tilikausi tilikausi);

protected:
    void lisaaTxt(const QString& txt);
    void kpAvattu();
    void saldot();

    void lisaaViimetiedostot();

    QWebEngineView *view;
    Sisalto *sisalto;
    Tilikausi tilikausi;

    QListWidget *viimelista;

    QTextBrowser *selain;
    QString teksti;
};

#endif // ALOITUSSIVU_H
