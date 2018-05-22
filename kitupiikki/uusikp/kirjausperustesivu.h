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

#ifndef KIRJAUSPERUSTESIVU_H
#define KIRJAUSPERUSTESIVU_H

#include <QWizardPage>

namespace Ui {
 class KirjausPeruste;
};

/**
 * @brief Kirjaamisperusteen valinta
 *
 * Tilikartan mukaan on mahdollista kirjanpitoa aloitettaessa valita pääasiallinen kirjaamisperuste:
 * suoriteperuste, laskuperuste tai maksuperuste. Tämän mukaan suoritetaan Kirjaamisperuste/XX skripti
 *
 */
class KirjausperusteSivu : public QWizardPage
{
public:
    KirjausperusteSivu();
    ~KirjausperusteSivu();

protected:
    Ui::KirjausPeruste *ui;
};

#endif // KIRJAUSPERUSTESIVU_H
