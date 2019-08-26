/*
   Copyright (C) 2019 Arto Hyvättinen

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
#ifndef APURIWIDGET_H
#define APURIWIDGET_H

#include <QWidget>

class QSortFilterProxyModel;

class Tosite;

class ApuriWidget : public QWidget
{
    Q_OBJECT
public:
    ApuriWidget(QWidget *parent, Tosite *pTosite);

    Tosite* tosite() { return pTosite_;}
    virtual void otaFokus() {;}

    virtual void tuo(QVariantMap map);

signals:

public slots:
    virtual void reset();
    virtual bool tositteelle();

protected:
    virtual void teeReset() = 0;
    virtual bool teeTositteelle() = 0;


    void aloitaResetointi();
    void lopetaResetointi();    

protected:
    Tosite* pTosite_;

private:
    bool resetointiKaynnissa_ = false;

};

#endif // APURIWIDGET_H
