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

#ifndef NIMISIVU_H
#define NIMISIVU_H

#include <QWizardPage>

#include "ui_nimi.h"

/**
 * @brief Uuden kirjanpidon velhon sivu, jossa kysytään kirjanpidon nimi
 */
class NimiSivu : public QWizardPage
{
protected:
    Ui::NimiSivu *ui;
public:
    NimiSivu();
    ~NimiSivu();

};

#endif // NIMISIVU_H
