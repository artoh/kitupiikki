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

#ifndef KITUPIIKKISIVU_H
#define KITUPIIKKISIVU_H

#include <QWidget>

/**
 * @brief Kantaluokka pääikkunassa näytettäville sivuille
 */
class KitupiikkiSivu : public QWidget
{
    Q_OBJECT
public:
    explicit KitupiikkiSivu(QWidget *parent = nullptr);

    /**
     * @brief Kutsutaan, kun ollaan siirtymässä toiselle sivulle
     * @return true, jos sivulta saa poistus
     */
    virtual bool poistuSivulta(int minne);

    /**
     * @brief Kutsutaan, kun tälle sivulle ollaan siirtymässä
     */
    virtual void siirrySivulle() {;}

    /**
     * @brief Sivu, jonne ohje ohjaa ;)
     * @return
     */
    virtual QString ohjeSivunNimi() { return ""; }

signals:


public slots:
};

#endif // KITUPIIKKISIVU_H
