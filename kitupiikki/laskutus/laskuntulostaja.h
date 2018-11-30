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
#include <QMap>

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
    explicit LaskunTulostaja(LaskuModel *model, const QString& kieli="FI");

signals:

public slots:
    bool tulosta(QPagedPaintDevice *printer, QPainter *painter, bool kaytaIkkunakuorta = true);
    void asetaKieli(const QString& kieli="FI");

public:
    QByteArray pdf(bool kaytaIkkunakuorta=true);

    QString html();

    QString virtuaaliviivakoodi() const;

    /**
     * @brief Lisää välin aina neljän merkin jälkeen
     * @param teksti
     * @return
     */
    static QString valeilla(const QString& teksti);

    /**
     * @brief Muotoilee viitenumeron ottaen huomioon RF:n
     * @return
     */
    QString muotoiltuViite() const;

    /**
     * @brief Palauttaa tekstin nykyisellä kielellä
     * @param avain Tekstin hakutunnus
     * @return Näytettävä teksti
     */
    QString t(const QString &avain) const;

    QString veroteksti(int verokoodi) const;

protected:
    void ylaruudukko(QPagedPaintDevice *printer, QPainter *painter, bool kaytaIkkunakuorta = true);
    qreal alatunniste(QPagedPaintDevice *printer, QPainter *painter);
    void erittely(LaskuModel* model, QPagedPaintDevice *printer, QPainter *painter, qreal marginaali);
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

    QString kieli_;
    QMap<QString,QString> tekstit_;
};

#endif // LASKUNTULOSTAJA_H
