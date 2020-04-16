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
#ifndef NAYTINVIEW_H
#define NAYTINVIEW_H

#include <QWidget>

#include "raportti/raportinkirjoittaja.h"

class QAction;
class QVBoxLayout;
class Esikatseltava;

namespace Naytin {
    class AbstraktiNaytin;
    class EsikatseluNaytin;
}


/**
 * @brief Widgetti liitteiden, raporttien jne esittämiseen
 */
class NaytinView : public QWidget
{
    Q_OBJECT
public:
    NaytinView(QWidget *parent = nullptr);
    ~NaytinView() override;

    QString otsikko() const;
    bool csvKaytossa() const;
    bool htmlKaytossa() const;
    bool raidatKaytossa() const;
    bool zoomKaytossa() const;

    QString tiedostonMuoto();
    QString tiedostoPaate();
    QByteArray csv();
    QByteArray data();
    QString html();

public slots:
    void nayta(const QByteArray& data);
    void nayta(RaportinKirjoittaja raportti);
    void nayta(const QString& teksti);

    Naytin::EsikatseluNaytin *esikatsele(Esikatseltava* katseltava);

    void paivita();
    void raidoita(bool raidat);
    void tulosta();
    void sivunAsetukset();
    void avaaOhjelmalla();
    void tallenna();

    void avaaHtml();
    void htmlLeikepoydalle();
    void tallennaHtml();

    void csvAsetukset();
    void tallennaCsv();
    void csvLeikepoydalle();

    void zoomFit();
    void zoomIn();
    void zoomOut();


signals:
    void sisaltoVaihtunut();


protected:
    void vaihdaNaytin(Naytin::AbstraktiNaytin* naytin);

    void contextMenuEvent(QContextMenuEvent *event) override;


    QAction* zoomAktio_;
    QAction* zoomInAktio_;
    QAction* zoomOutAktio_;
    QAction* tulostaAktio_;
    QAction* tallennaAktio_;
    QAction* avaaAktio_;

    QVBoxLayout *leiska_;
    Naytin::AbstraktiNaytin *naytin_ = nullptr;

    static QString viimeisinPolku__;

};

#endif // NAYTINVIEW_H
