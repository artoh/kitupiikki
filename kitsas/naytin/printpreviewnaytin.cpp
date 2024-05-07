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
#include <QDebug>
#include <QPrinterInfo>
#include <QPdfWriter>
#include <QPageSize>
#include <QSettings>

Naytin::PrintPreviewNaytin::PrintPreviewNaytin(QWidget *parent)
    : AbstraktiNaytin (parent)
{    
    // Alustetaan printteri
    // Vähimmäismarginaalit 1 cm joka suuntaan


    QMarginsF margins = kp()->printer()->pageLayout().margins(QPageLayout::Millimeter);
    if( margins.top() < 10)
        margins.setTop(10);
    if( margins.left() < 10)
        margins.setLeft(10);
    if( margins.right() < 10)
        margins.setRight(10);
    if( margins.bottom() < 10)
        margins.setBottom(10);

    printer_ = new QPrinter(QPrinter::HighResolution);        
    printer_->setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, margins, QPageLayout::Millimeter));

    widget_ = new QPrintPreviewWidget(printer_, parent);

    qreal zoomFactor = kp()->settings()->value("PrintPreviewZoomFactor").toDouble();
    int mode = kp()->settings()->value("PrintPreviewZoomMode").toInt();
    if( mode == QPrintPreviewWidget::ZoomMode::FitInView)
        widget_->setZoomMode(QPrintPreviewWidget::ZoomMode::FitInView);
    else if( mode == QPrintPreviewWidget::ZoomMode::CustomZoom && zoomFactor > 0.1) {
        widget_->setZoomMode(QPrintPreviewWidget::ZoomMode::CustomZoom);
        widget_->setZoomFactor( zoomFactor );
    }

    connect( widget_, &QPrintPreviewWidget::paintRequested, this, &PrintPreviewNaytin::tulosta );
}

Naytin::PrintPreviewNaytin::~PrintPreviewNaytin()
{
    kp()->settings()->setValue("PrintPreviewZoomFactor", widget_->zoomFactor());
    kp()->settings()->setValue("PrintPreviewZoomMode", widget_->zoomMode());

    delete printer_;
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

QPrinter *Naytin::PrintPreviewNaytin::printer()
{
    return printer_;
}
