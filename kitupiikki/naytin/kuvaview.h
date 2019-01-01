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

#include <QGraphicsView>
#include <QImage>

namespace Naytin {

class KuvaView : public QGraphicsView
{
public:
    KuvaView(const QImage& kuva);

    QImage kuva() const;

public slots:
    void paivita();

    virtual void zoomIn();
    virtual void zoomOut();
    virtual void zoomFit();

protected:
    void resizeEvent(QResizeEvent *event) override;

protected:
    QImage kuva_;
    double zoomaus_ = 1.00;

};


}


#endif // KUVAVIEW_H
