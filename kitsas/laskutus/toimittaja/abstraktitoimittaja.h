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
#ifndef ABSTRAKTITOIMITTAJA_H
#define ABSTRAKTITOIMITTAJA_H

#include <QObject>
#include <QQueue>
#include <QTimer>

#include "model/tosite.h"

class AbstraktiToimittaja : public QObject
{
    Q_OBJECT
public:
    explicit AbstraktiToimittaja(QObject *parent = nullptr);
    virtual ~AbstraktiToimittaja();

    void toimitaLasku(Tosite* tosite);

    virtual void toimita() = 0;

signals:
    void onnistuiMerkitty();
    void onnistui(int tositeId);
    void epaonnistui(const QString& kuvaus);

protected:
    void valmis(bool merkitty = false);
    void virhe(const QString& kuvaus);

    Tosite* tosite() { return pTosite_;}

private:
    Tosite* pTosite_ = nullptr;

};

#endif // ABSTRAKTITOIMITTAJA_H
