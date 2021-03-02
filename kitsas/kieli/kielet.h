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
#ifndef KIELET_H
#define KIELET_H

#include <QObject>
#include <QHash>
#include "kieli.h"

#include "monikielinen.h"

class Kielet final
{    

private:
    Kielet(const QString& tiedostonnimi);

public:
    static void alustaKielet(const QString& kaannostiedostonnimi);
    static Kielet* instanssi();

public slots:
    void asetaKielet(const QString &json);
    void valitseKieli(const QString &kieli);

public:
    QString kaanna(const QString &avain, const QString &kieli = QString())  const;
    QList<Kieli> kielet() const;
    QString nykyinen() const;

private:
    QHash<QString,QMap<QString,QString>> kaannokset_;
    QList<QPair<QString,Monikielinen>> kielet_;
    QString nykykieli_;

private:
    static Kielet* instanssi__;

};

#endif // KIELET_H
