/*
   Copyright (C) 2018 Arto Hyvättinen

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

#ifndef SKRIPTI_H
#define SKRIPTI_H

#include <QStringList>

class AsetusModel;
class TiliModel;
class TositelajiModel;

/**
 * @brief Suorittaa monimuotoisuuteen liittyvän Kitupiikin skriptin
 *
 * Skriptit on tallennettu asetuksiin
 *
 * LuontiSkripti
 * PaivitysSkripti
 *
 * MuotoOn/muoto
 * MuotoPois/muoto
 * MuotoPaivitys/muoto
 *
 * Kirjausperuste/Suoriteperuste
 * Kirjausperuste/Laskuperuste
 * Kirjausperuste/Maksuperuste
 *
 * Skripteissä komentoja
 * +2001..2005  Tilit käyttöön
 * -2005..2007  Tilit pois käytöstä
 *
 * Avain=Asetus
 * Avain+=Asetus
 * Avaon-=Asetus
 *
 * OL/1910  Tositelajin oletusvastatili
 *
 */
class Skripti
{
protected:
    Skripti();
    Skripti(AsetusModel* asetusModel, TiliModel* tiliModel, TositelajiModel *lajimodel);
    void suorita();

    QStringList skripti_;
    AsetusModel *asetusModel_;
    TiliModel* tiliModel_;
    TositelajiModel *tositelajiModel_;

public:
    static void suorita(const QString& skriptinnimi);
    static void suorita(const QStringList &skripti, AsetusModel* asetusModel, TiliModel* tiliModel, TositelajiModel *lajimodel);
    static void suorita(const QStringList& skripti);
};

#endif // SKRIPTI_H
