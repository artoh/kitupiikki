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
#ifndef PILVIYHTEYS_H
#define PILVIYHTEYS_H

#include "db/kpyhteys.h"
#include "pilvikysely.h"

class PilviYhteys : public KpYhteys
{
    Q_OBJECT
public:
    PilviYhteys(QObject* parent, int pilviId, QString osoite, QString token);

    PilviKysely *kysely(const QString& polku = QString(), KpKysely::Metodi metodi = KpKysely::GET) override;

    QString pilviosoite() const { return pilviosoite_;}
    QString token() const { return token_;}
    int pilviId() const { return pilviId_; }

public slots:
    void alustaYhteys();

protected slots:
    void initSaapui(QVariant *reply, int tila);

protected:
    int pilviId_;
    QString pilviosoite_;
    QString token_;
};

#endif // PILVIYHTEYS_H
