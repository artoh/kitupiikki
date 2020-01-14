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
#include "printpreviewnaytin.h"

#include <QPrintPreviewWidget>
#include "db/kirjanpito.h"

Naytin::PrintPreviewNaytin::PrintPreviewNaytin(QObject *parent)
    : AbstraktiNaytin (parent), widget_{ new QPrintPreviewWidget( kp()->printer() ) }
{
    connect( widget_, &QPrintPreviewWidget::paintRequested, this, &PrintPreviewNaytin::tulosta );
}

Naytin::PrintPreviewNaytin::~PrintPreviewNaytin()
{
    widget_->deleteLater();
}

QWidget *Naytin::PrintPreviewNaytin::widget()
{
    return widget_;
}

void Naytin::PrintPreviewNaytin::paivita() const
{
    widget_->updatePreview();
}

void Naytin::PrintPreviewNaytin::zoomIn()
{
    widget_->zoomIn();
}

void Naytin::PrintPreviewNaytin::zoomOut()
{
    widget_->zoomOut();
}

void Naytin::PrintPreviewNaytin::zoomFit()
{
    widget_->fitToWidth();
}
