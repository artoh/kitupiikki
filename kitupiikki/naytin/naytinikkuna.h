/*
   Copyright (C) 2018 Arto Hyvättinen

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
#ifndef NAYTINIKKUNA_H
#define NAYTINIKKUNA_H


#include "raportti/raportinkirjoittaja.h"
#include <QMainWindow>

class NaytinView;
class QAction;



/**
 * @brief Ikkuna kaikenlaiseen näyttämiseen
 */
class NaytinIkkuna : public QMainWindow
{
    Q_OBJECT
public:
    NaytinIkkuna(QWidget *parent = nullptr);
    ~NaytinIkkuna();

    NaytinView* view() { return view_;}

    static void naytaRaportti(const RaportinKirjoittaja &raportti);
    static void nayta(const QByteArray &data);
    static void naytaTiedosto(const QString& tiedostonnimi);
    static void naytaLiite(const int tositeId, const int liiteId);

private slots:
    void sisaltoMuuttui();

private:
    void teeToolbar();

private:
    NaytinView *view_;

    QAction *avaaAktio_;
    QAction *raitaAktio_;
    QAction *sivunAsetusAktio_;

    QAction *csvAktio_;
    QAction *htmlAktio_;



};

#endif // NAYTINIKKUNA_H
