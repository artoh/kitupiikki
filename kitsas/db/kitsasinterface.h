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
#ifndef KITSASINTERFACE_H
#define KITSASINTERFACE_H

#include <QString>
#include <QDate>
#include <QImage>

class AsetusModel;
class TiliModel;
class TilikausiModel;
class KohdennusModel;
class TuoteModel;
class RyhmatModel;
class AlvIlmoitustenModel;
class KiertoModel;
class QSettings;
class YhteysModel;
class TositeTyyppiModel;
class VakioViiteModel;
class HuoneistoModel;

/**
 * @brief Rajapinta keskeisten modeleiden saavuttamiseen
 *
 * Koska Kirjanpito-ainokainen aiheuttaa niin paljon
 * riippuvuuksia, toteutetaan sitä varten rajapinta, jotta
 * uudemmissa toteutuksissa voidaan testejä varten
 * sisällyttää modelit vain osittain
 *
 */
class KitsasInterface {

public:
    virtual ~KitsasInterface() {}

    virtual AsetusModel* asetukset() const { return nullptr;}
    virtual TiliModel* tilit() const { return nullptr;}
    virtual TilikausiModel* tilikaudet() const { return nullptr;}
    virtual KohdennusModel* kohdennukset() const { return nullptr;}
    virtual TuoteModel* tuotteet() const { return nullptr;}
    virtual RyhmatModel* ryhmat() const { return nullptr;}
    virtual AlvIlmoitustenModel* alvIlmoitukset() const { return nullptr;}
    virtual KiertoModel* kierrot() const { return nullptr;}
    virtual QSettings* settings() const { return nullptr;}
    virtual YhteysModel* yhteysModel() { return nullptr;}
    virtual TositeTyyppiModel* tositeTyypit() const { return nullptr;}
    virtual VakioViiteModel* vakioViitteet() const { return nullptr;}
    virtual HuoneistoModel* huoneistot() const { return nullptr;}


    virtual QString tositeTunnus(int tunniste, const QDate& pvm, const QString& sarja, bool samakausi = false, bool vertailu = false) const;
    virtual QString kaanna(const QString& teksti, const QString& kieli = QString()) const;
    virtual QDate paivamaara() const;
    virtual QImage logo() const;
    virtual bool onkoHarjoitus() const { return true; }
    virtual bool onkoMaksuperusteinenAlv(const QDate& paiva) const;

};

inline QString KitsasInterface::tositeTunnus(int /*tunniste*/, const QDate &/*pvm*/, const QString &/*sarja*/, bool /*samakausi*/, bool /*vertailu*/) const { return QString();}
inline QString KitsasInterface::kaanna(const QString &teksti, const QString &/*kieli*/) const {return teksti; }
inline QDate KitsasInterface::paivamaara() const { return QDate::currentDate(); }
inline QImage KitsasInterface::logo() const { return QImage();}
inline bool KitsasInterface::onkoMaksuperusteinenAlv(const QDate &/*paiva*/) const { return false; }

#endif // KITSASINTERFACE_H
