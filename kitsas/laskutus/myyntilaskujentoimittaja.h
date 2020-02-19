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
#ifndef MYYNTILASKUJENTOIMITTAJA_H
#define MYYNTILASKUJENTOIMITTAJA_H

#include <QObject>
#include <QList>
#include <QVariantMap>

class VerkkolaskuToimittaja;

class MyyntiLaskujenToimittaja : public QObject
{
    Q_OBJECT

public:
    MyyntiLaskujenToimittaja(QObject *parent = nullptr);
    bool toimitaLaskut( const QList<QVariantMap>& laskut );

    void toimitaLaskut( const QList<int>& tositteet);

signals:
    void laskutToimitettu();

protected slots:
    void toimitettu();
    void virhe();

protected slots:
    void tositeSaapuu(QVariant* data);
    void tositeTallennettu(QVariant* data);

    void lahetaSeuraava(int status);


protected:
    void tarkistaValmis();
    bool tulosta();
    bool tallenna();
    QString maksutiedot(const QVariantMap& data);
    void merkkaaToimitetuksi(int tositeid);

    void tilaaSeuraavaLasku();

protected:
    QList<QVariantMap> toimitettavat_;
    QList<int> tilattavat_;
    QList<QVariantMap> tulostettavat_;
    QList<QVariantMap> tallennettavat_;
    QList<QVariantMap> sahkopostilla_;
    VerkkolaskuToimittaja* verkkolaskutoimittaja_;
    int toimitetut_ = 0;
    int virheita_ = 0;
    int laskuja_ = 0;    
    bool emailvirheita_ = false;
};

#endif // MYYNTILASKUJENTOIMITTAJA_H
