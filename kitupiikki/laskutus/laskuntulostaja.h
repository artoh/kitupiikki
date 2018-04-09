/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef LASKUNTULOSTAJA_H
#define LASKUNTULOSTAJA_H

#include <QObject>
#include <QPainter>
#include <QPrinter>
#include <QFile>

#include "laskumodel.h"

/**
 * @brief Laskuntulostajan alv-erittelyä varten
 */
struct AlvErittelyEra
{
    AlvErittelyEra() {}

    double netto = 0;
    double vero = 0;
    double brutto() const { return netto + vero; }
};


/**
 * @brief Laskun tulostuksen luokka
 */
class LaskunTulostaja : public QObject
{
    Q_OBJECT
public:
    explicit LaskunTulostaja(LaskuModel *model);

signals:

public slots:
    bool tulosta(QPagedPaintDevice *printer);
    bool kirjoitaPdf(QString tiedostonnimi);

public:
    QString html();

    QString virtuaaliviivakoodi() const;

    /**
     * @brief Lisää välin aina neljän merkin jälkeen
     * @param teksti
     * @return
     */
    static QString valeilla(const QString& teksti);

protected:
    void ylaruudukko(QPagedPaintDevice *printer, QPainter *painter);
    void lisatieto(QPainter *painter);
    qreal alatunniste(QPagedPaintDevice *printer, QPainter *painter);
    void erittely(QPagedPaintDevice *printer, QPainter *painter, qreal marginaali);
    void erittelyOtsikko(QPagedPaintDevice *printer, QPainter *painter, bool alv);

    void tilisiirto(QPagedPaintDevice *printer, QPainter *painter);
    QString iban;

    /**
     * @brief Pankkiviivakoodi valmiiksi code128-koodattuna
     * @return
     */
    QString code128() const;
    /**
     * @brief Code 128C viivakoodin merkki joka vastaa annettua numeroparia
     * @param koodattava
     * @return
     */
    QChar code128c(int koodattava) const;

    /**
     * @brief QR-koodi SVG-muodossa
     * @return
     */
    QByteArray qrSvg() const;

private:
    LaskuModel *model_;

};

#endif // LASKUNTULOSTAJA_H
