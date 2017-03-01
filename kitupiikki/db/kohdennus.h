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

/**
 * @brief Kirjauksen kohdennus kustannuspaikalle tai projektiin
 */

class Kohdennus
{
public:

    enum KohdennusTyyppi
    {
        EIKOHDENNETA = 0,
        KUSTANNUSPAIKKA = 1,
        PROJEKTI = 2
    };

    Kohdennus(int tyyppi = EIKOHDENNETA, const QString& nimi = QString());
    Kohdennus(int id, int tyyppi, QString nimi, QDate alkaa = QDate(), QDate paattyy = QDate());

    int id() const { return id_; }
    QString nimi() const { return nimi_; }
    QDate alkaa() const { return alkaa_; }
    QDate paattyy() const { return paattyy_; }
    int tyyppi() const { return tyyppi_; }
    QIcon tyyppiKuvake() const;

    bool muokattu() const { return muokattu_; }

    /**
     * @brief Montako kirjausta tälle kohdennukselle
     * @return
     */
    int montakoVientia() const;

    void asetaId(int id);
    void asetaNimi(const QString& nimi);
    void asetaAlkaa(const QDate& alkaa);
    void asetaPaattyy(const QDate& paattyy);
    void asetaTyyppi(KohdennusTyyppi tyyppi);
    void nollaaMuokattu();

protected:
    int id_;
    int tyyppi_;
    QString nimi_;
    QDate alkaa_;
    QDate paattyy_;

    bool muokattu_;
};

#endif // KOHDENNUS_H
