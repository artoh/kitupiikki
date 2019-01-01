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
#ifndef PRINTPREVIEWNAYTIN_H
#define PRINTPREVIEWNAYTIN_H

#include "abstraktinaytin.h"

class QPrintPreviewWidget;

namespace Naytin {

/**
 * @brief QPrintPreviewWidgetillä toteutettujen näyttimien kantaluokka
 */
class PrintPreviewNaytin : public AbstraktiNaytin
{
    Q_OBJECT
public:
    PrintPreviewNaytin(QObject* parent = nullptr);
    virtual ~PrintPreviewNaytin() override;

    virtual QWidget* widget() override;

public slots:
    virtual void paivita() const override;

protected:
    QPrintPreviewWidget *widget_ ;
};

}

#endif // PRINTPREVIEWNAYTIN_H
