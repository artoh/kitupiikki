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
#ifndef TULOMENOAPURITESTI_H
#define TULOMENOAPURITESTI_H

#include <QTest>


class TulomenoApuriTesti : public QObject
{
    Q_OBJECT
public:
    TulomenoApuriTesti(QObject *parent = nullptr);

signals:

private slots:

    void initTestCase();
    void init();
    void clean();

    void yksinkertainenTulotosite();
    void verollinenTulotosite();

    void verollinenTuloKirjausWglla();
    void verollinenTuloKirjausSivulla();


protected:
    QVariant verollinenTuloTosite_;


};

#endif // TULOMENOAPURITESTI_H
