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
#ifndef TUONTITULKKI_H
#define TUONTITULKKI_H

#include "../sqliteroute.h"

class TuontiTulkki : public SQLiteRoute
{
public:
    TuontiTulkki( SQLiteModel *model);

    QVariant post(const QString &polku, const QVariant &data) override;

protected:
    QVariant tiliote(QVariantMap &map);

    void tilioteTulorivi(QVariantMap& rivi);

    void tilioteMenorivi(QVariantMap& rivi);

    QPair<int, QString> kumppaniNimella(const QString& nimi);
    QPair<int, QString> kumppaniIbanilla(const QString& iban);
};

#endif // TUONTITULKKI_H
