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

#ifndef PAIVITAKIRJANPITO_H
#define PAIVITAKIRJANPITO_H

#include <QObject>


/**
 * @brief Kirjanpidon päivitykseen liittyvät luokat
 */
class PaivitaKirjanpito : public QObject
{

public:
    explicit PaivitaKirjanpito(QObject *parent = nullptr) =delete;

    /**
     * @brief Jos tälle tilikartalle saatavissa sisäinen päivitys,
     * niin kyseisen päivityksen kuvaileva teksti
     * @return
     */
    static QString sisainenPaivitys();
    static void paivitaTilikartta();

protected:
    static void lataaPaivitys(const QString& tiedosto);


};

#endif // PAIVITAKIRJANPITO_H
