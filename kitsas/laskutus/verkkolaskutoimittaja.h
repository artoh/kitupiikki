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
#ifndef VERKKOLASKUTOIMITTAJA_H
#define VERKKOLASKUTOIMITTAJA_H

#include <QObject>
#include <QQueue>
#include <QVariantMap>

class VerkkolaskuToimittaja : public QObject
{
    Q_OBJECT
public:
    explicit VerkkolaskuToimittaja(QObject *parent = nullptr);

    void lisaaLasku(const QVariantMap &lasku);
    bool toimitaSeuraava();

signals:
    void toimitettu(int tositeId);
    void toimitusEpaonnistui();

protected:
    void alustaInit();
    void asiakasSaapuu(const QVariant* data, const QVariantMap &map);
    void laskuSaapuu(QVariant* data, int tositeId, int laskuId);
    void virhe(const QString& viesti);


protected:
    QQueue<QVariantMap> laskut_;
    QVariantMap init_;
    bool virhe_ = false;

};

#endif // VERKKOLASKUTOIMITTAJA_H
