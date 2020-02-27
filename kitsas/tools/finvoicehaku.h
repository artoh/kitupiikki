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
#ifndef FINVOICEHAKU_H
#define FINVOICEHAKU_H

#include <QObject>
#include <QVariantMap>
#include <QDateTime>

class Kirjanpito;

class Tosite;

class FinvoiceHaku : public QObject
{
    Q_OBJECT
public:
    static FinvoiceHaku *init(Kirjanpito* kp);
    void haeUudet();
    static FinvoiceHaku *instanssi();

protected:
    explicit FinvoiceHaku(QObject *parent = nullptr);

protected:
    void kpVaihtui();
    void listaSaapuu(QVariant *data);
    void haeSeuraava();
    void jsonSaapuu(QVariant *data);
    void kumppaniSaapuu(QVariant *data, const QVariantMap json);
    void kuvaSaapuu(QVariant *data);
    void xmlSaapuu(QVariant *data);
    void tallennettu();


    Tosite* nykyTosite_;

    QVariantMap nykyinen_;
    QVariantList hakulista_;
    int haettuLkm_;
    bool hakuPaalla_ = false;
    QString ytunnus_;
    QDateTime aikaleima_;

    static FinvoiceHaku* instanssi__;
signals:


};

#endif // FINVOICEHAKU_H
