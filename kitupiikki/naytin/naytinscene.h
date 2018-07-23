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
#ifndef NAYTINSCENE_H
#define NAYTINSCENE_H


#include <QGraphicsScene>

class QPrinter;

/**
 * @brief Scenen kantaluokka Nayttimeen
 */
class NaytinScene : public QGraphicsScene
{
    Q_OBJECT
public:
    NaytinScene(QObject *parent = nullptr);

    virtual QString otsikko() const { return QString(); }

    virtual void piirraLeveyteen(double leveyteen) = 0;

    virtual QString tyyppi() const = 0;

    virtual bool csvMuoto() { return false; }
    virtual QByteArray csv() { return QByteArray(); }

    virtual QString tiedostonMuoto() = 0;
    virtual QString tiedostoPaate() = 0;
    virtual QByteArray data() = 0;
    virtual QString html() { return QString(); }

    virtual void tulosta(QPrinter* printer) = 0;

    virtual bool raidoita(bool raidat = false);

    virtual bool sivunAsetuksetMuuttuneet() { return false;}

protected:

};

#endif // NAYTINSCENE_H
