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
#ifndef RIVIVIENTIGENEROIJA_H
#define RIVIVIENTIGENEROIJA_H

#include "../tositerivialv.h"

class Tosite;
class KitsasInterface;

class RiviVientiGeneroija
{
public:
    RiviVientiGeneroija(KitsasInterface* kitsas);

    void generoiViennit(Tosite* tosite);

protected:
    void asetaEraId();

    void generoiKuukausittaisetLaskut();
    void generoiViennit(const QDate& pvm);
    void generoiVastavienti(const QDate& pvm);
    void generoiTiliviennit(const QDate& pvm);
    void generoiVeroviennit(const QDate& pvm);
    void generoiVeroVienti(const double prosentti, const Euro& vero, const QDate& pvm);


private:
    TositeriviAlv alv_;
    KitsasInterface *kitsas_;

    Tosite* tosite_ = nullptr;

    int eraId_ = 0;
};

#endif // RIVIVIENTIGENEROIJA_H
