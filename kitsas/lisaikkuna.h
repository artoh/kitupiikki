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

#ifndef LISAIKKUNA_H
#define LISAIKKUNA_H

#include <QMainWindow>

#include "kitupiikkiikkuna.h"
#include "db/tositetyyppimodel.h"

/**
 * @brief Toinen ikkuna selaamista tai kirjaamista varten
 */
class LisaIkkuna : public QMainWindow
{
    Q_OBJECT
public:
    LisaIkkuna(QWidget *parent = nullptr);
    ~LisaIkkuna();

    KirjausSivu *kirjaa(int tositeId = -1, int tyyppi = TositeTyyppi::TULO);
    void selaa();

signals:

public slots:
    /**
     * @brief Näyttää tositteen uudessa ikkunassa
     * @param tositeId
     */
    void naytaTosite(int tositeId);

    /**
     * @brief Näyttää ohjeen
     */
    void ohje();


private:
    QString ohjesivu;
};

#endif // LISAIKKUNA_H
