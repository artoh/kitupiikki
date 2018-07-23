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

/**
 * @brief Widgetti liitteiden, raporttien jne esittämiseen
 */
class NaytinView : public QGraphicsView
{
    Q_OBJECT
public:
    NaytinView(QWidget *parent = nullptr);

public slots:
    void nayta(const QByteArray& data);
    void nayta(RaportinKirjoittaja raportti);
    void sivunAsetuksetMuuttuneet();
    void paivita();

    QString otsikko() const;

    bool csvKaytossa() const;


    QByteArray csv();
    QString tiedostonMuoto();
    QString tiedostoPaate();
    QByteArray data();

signals:
    void sisaltoVaihtunut(const QString& tyyppi);

protected:
    void vaihdaScene(NaytinScene* uusi);
    void resizeEvent(QResizeEvent *event) override;


    NaytinScene *scene_ = nullptr;
};

#endif // NAYTINVIEW_H
