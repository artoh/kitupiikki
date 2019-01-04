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
#ifndef SCENENAYTIN_H
#define SCENENAYTIN_H

#include "abstraktinaytin.h"
#include <QImage>

#include "abstraktiview.h"

namespace Naytin {

class SceneNaytin : public AbstraktiNaytin
{
public:
    SceneNaytin(Naytin::AbstraktiView *view, QObject *parent = nullptr);

    QWidget* widget() override;

    QString tiedostonMuoto() const override;
    QString tiedostonPaate() const override;
    QString otsikko() const override;

    QByteArray data() const override;

    bool voikoZoomata() const override { return true; }

public slots:
    void paivita() const override;
    void tulosta(QPrinter* printer) const override;

    virtual void zoomIn() override;
    virtual void zoomOut() override;
    virtual void zoomFit() override;

protected:
   Naytin::AbstraktiView *view_;

};

}



#endif // SCENENAYTIN_H
