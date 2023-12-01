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
#ifndef TILIOTERIVI_H
#define TILIOTERIVI_H

#include <QString>
#include <QDate>
#include <QVariant>

class TilioteModel;

class TilioteRivi
{
public:
    virtual ~TilioteRivi();

    enum Sarakkeet {
        PVM, SAAJAMAKSAJA, SELITE, TILI, ALV, KOHDENNUS, EURO
    };

    enum {
        LajitteluRooli = Qt::UserRole + 1,
        TilaRooli = Qt::UserRole + 2,
        TositeIdRooli = Qt::UserRole + 3,
        LisaysIndeksiRooli = Qt::UserRole + 4,
        EraIdRooli = Qt::UserRole + 5,
        EuroRooli = Qt::UserRole + 6,
        PvmRooli = Qt::UserRole + 7,
        TiliRooli = Qt::UserRole + 8,
    };


    TilioteRivi();
    TilioteRivi(TilioteModel* model);

    int lisaysIndeksi() const { return lisaysIndeksi_;}
    virtual void asetaLisaysIndeksi(const int indeksi);

protected:
    TilioteModel* model() const { return model_;}


private:
    TilioteModel* model_ = nullptr;
    int lisaysIndeksi_ = 0;
};

#endif // TILIOTERIVI_H
