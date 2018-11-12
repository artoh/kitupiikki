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

#include <QGraphicsView>
#include "naytinscene.h"
#include "raportti/raportinkirjoittaja.h"

class QAction;
class QTextEdit;
class QStackedLayout;

/**
 * @brief Widgetti liitteiden, raporttien jne esittämiseen
 */
class NaytinView : public QWidget
{
    Q_OBJECT
public:
    NaytinView(QWidget *parent = nullptr);

    enum {
      Scene = 0,
      Editor = 1
    };

public slots:
    void nayta(const QByteArray& data);
    void nayta(const RaportinKirjoittaja &raportti);
    void sivunAsetuksetMuuttuneet();
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

    QString otsikko() const;
    bool csvKaytossa() const;

    QString tiedostonMuoto();
    QString tiedostoPaate();
    QByteArray csv();
    QByteArray data();
    QString html();

signals:
    void sisaltoVaihtunut(const QString& tyyppi);


protected:
    void vaihdaScene(NaytinScene* uusi);
    void resizeEvent(QResizeEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    double zoomaus_ = 1.00;
    NaytinScene *scene_ = nullptr;

    QAction* zoomAktio_;
    QAction* zoomInAktio_;
    QAction* zoomOutAktio_;
    QAction* tulostaAktio_;
    QAction* tallennaAktio_;

    QStackedLayout* leiska_;
    QGraphicsView* view_;
    QTextEdit *editor_;

};

#endif // NAYTINVIEW_H
