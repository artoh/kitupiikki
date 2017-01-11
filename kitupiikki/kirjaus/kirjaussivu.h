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

#ifndef KIRJAUSSIVU_H
#define KIRJAUSSIVU_H

#include <QWidget>

class KirjausWg;
class TositeWg;
class Kirjanpito;

class KirjausSivu : public QWidget
{
    Q_OBJECT
public:
    KirjausSivu(Kirjanpito* kirjanpito);
    ~KirjausSivu();

signals:

public slots:

protected:
    KirjausWg *kirjauswg;
    TositeWg *tositewg;
};

#endif // KIRJAUSSIVU_H
