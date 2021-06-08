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
#ifndef ERAMAP_H
#define ERAMAP_H

#include <QVariantMap>
#include <QDate>
#include "euro.h"

class EraMap : public QVariantMap
{
public:
    enum EraTyyppi { Valitse = -2, Uusi = -1, EiEraa = 0, Asiakas = -3, Huoneisto = -4, Lasku = 2};

    EraMap();
    EraMap(int id);
    EraMap(const QVariantMap& map);

    int id() const { return value("id").toInt(); }
    QDate pvm() const { return value("pvm").toDate(); }
    QString nimi() const { return value("selite").toString(); }
    QString kumppaniNimi() const { return value("kumppani").toMap().value("nimi").toString(); }
    QVariantMap kumppani() const { return value("kumppani").toMap(); }
    int kumppaniId() const { return value("kumppani").toMap().value("id").toInt(); }
    int tositetyyppi() const { return value("tositetyyppi").toInt(); }
    Euro saldo() const;
    QString asiakasNimi() const { return value("asiakas").toMap().value("nimi").toString();}
    QString huoneistoNimi() const { return value("huoneisto").toMap().value("nimi").toString();}
    int huoneistoId() const { return eratyyppi() == Huoneisto ?  id() / -10 : 0; }
    int asiakasId() const { return eratyyppi() == Asiakas ? id() / -10 : 0; }

    int tunniste() const { return value("tunniste").toInt(); }
    QString sarja() const { return value("sarja").toString(); }

    int eratyyppi() const;

    static EraMap AsiakasEra(int id, const QString& nimi);
};

#endif // ERAMAP_H
