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
#ifndef KUVAVIEW_H
#define KUVAVIEW_H

#include "abstraktiview.h"
#include <QImage>

namespace Naytin {

class KuvaView : public AbstraktiView
{
public:
    KuvaView(const QImage& kuva);

    virtual QString tiedostonMuoto() const override { return tr("jpg-kuva (*.jpg)");}
    virtual QString tiedostonPaate() const override { return "jpg"; }

    virtual QByteArray data() const override;

public slots:
    void paivita() const override;
    void tulosta(QPrinter *printer) const override;

protected:
    QImage kuva_;
};


}


#endif // KUVAVIEW_H
