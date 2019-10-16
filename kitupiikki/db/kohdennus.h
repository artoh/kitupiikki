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

#ifndef KOHDENNUS_H
#define KOHDENNUS_H

#include <QString>
#include <QDate>
#include <QIcon>

#include "kantavariantti.h"
#include "kielikentta.h"

/**
 * @brief Kirjauksen kohdennus kustannuspaikalle tai projektiin
 */

class Kohdennus : protected KantaVariantti
{
public:

    enum KohdennusTyyppi
    {
        EIKOHDENNETA = 0,
        KUSTANNUSPAIKKA = 1,
        PROJEKTI = 2,
        MERKKAUS = 3
    };

    Kohdennus( int tyyppi = EIKOHDENNETA);
    Kohdennus( QVariantMap& data);

    int id() const { return id_; }
    QString nimi(const QString& kieli = QString()) const { return nimi_.teksti(kieli); }
    QString kaannos(const QString& kieli) const { return nimi_.kaannos(kieli); }
    QDate alkaa() const { return pvm("alkaa"); }
    QDate paattyy() const { return pvm("paattyy"); }
    int tyyppi() const { return tyyppi_; }
    int kuuluu() const { return kuuluu_; }
    int montakoVientia() const { return vienteja_; }
    QIcon tyyppiKuvake() const;


    void asetaId(int id);
    void asetaNimi( const QString& nimi, const QString& kieli );
    void asetaTyyppi(KohdennusTyyppi tyyppi);
    void asetaKuuluu(int kohdennusid);
    void asetaAlkaa(const QDate& pvm) { set("alkaa", pvm); }
    void asetaPaattyy(const QDate& pvm) { set("paattyy", pvm);}

    QVariantMap data() const;

protected:
    int id_=0;
    int tyyppi_=0;
    KieliKentta nimi_;
    int kuuluu_=0;
    int vienteja_=0;
};

#endif // KOHDENNUS_H
