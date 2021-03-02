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

#include "abstraktikielet.h"
#include "monikielinen.h"

#include <QHash>

class Kielet : public AbstraktiKielet
{
public:
    Kielet(const QString& tiedostonnimi);

public:
    virtual void asetaKielet(const QString &json) override;
    virtual void valitseKieli(const QString &kieli) override;
    virtual QString kaanna(const QString &avain, const QString &kieli = QString())  const override;
    virtual QList<Kieli> kielet() const override;
    virtual QString nykyinen() const override;

private:
    QHash<QString,QMap<QString,QString>> kaannokset_;
    QList<QPair<QString,Monikielinen>> kielet_;
    QString nykykieli_;

};

#endif // KIELET_H
