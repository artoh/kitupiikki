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
#ifndef MYYNTILASKUNTULOSTAJA_H
#define MYYNTILASKUNTULOSTAJA_H

#include <QObject>
#include <QHash>

class QPagedPaintDevice;
class QPainter;

#include "laskurivitmodel.h"

class MyyntiLaskunTulostaja : public QObject
{
    Q_OBJECT
public:
    MyyntiLaskunTulostaja(const QVariantMap& map, QObject *parent = nullptr);

    static bool tulosta(const QVariantMap& lasku,
                 QPagedPaintDevice *printer,
                 QPainter *painter,
                 bool kuoreen = true);

    static QByteArray pdf(const QVariantMap& lasku, bool ikkunakuoreen = false);
    static QString virtuaaliviivakoodi(const QVariantMap& lasku);

    static QString valeilla(const QString& teksti);
    static QString bicIbanilla(const QString& iban);
    QString t(const QString& avain) const;


    QString virtuaaliviivakoodi() const;
    QString muotoiltuViite() const;
    QString iban() const { return valeilla(iban_);}

signals:

public slots:


protected:    

    void tulosta(QPagedPaintDevice *printer,
                 QPainter *painter,
                 bool kuoreen);

    void ylaruudukko(QPagedPaintDevice *printer, QPainter *painter, bool kaytaIkkunakuorta);
    void tilisiirto(QPagedPaintDevice *printer, QPainter *painter);
    qreal alatunniste(QPagedPaintDevice *printer, QPainter *painter);


    QByteArray qrSvg() const;
    QString code128() const;



protected:

    void tekstiRivinLisays(const QString &rivi, const QString &kieli = QString());
    QChar code128c(int koodattava) const;


    QHash<QString,QString> tekstit_;
    QVariantMap map_;
    LaskuRivitModel rivit_;
    double laskunSumma_;

    QString iban_;
    QStringList ibanit_;

};

#endif // MYYNTILASKUNTULOSTAJA_H
