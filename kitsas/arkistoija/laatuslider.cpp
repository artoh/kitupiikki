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
#include "laatuslider.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSettings>

#include "db/kirjanpito.h"

LaatuSlider::LaatuSlider(QWidget *parent) : QWidget(parent)
{
    slider = new QSlider();
    label = new QLabel();

    slider->setOrientation(Qt::Horizontal);
    slider->setMinimum(4);
    slider->setMaximum(16);

    slider->setSingleStep(1);
    slider->setPageStep(2);

    slider->setTickPosition(QSlider::TicksBothSides);
    slider->setTickInterval(2);

    dpi_ = kp()->settings()->value("AineistoLaatu", 175).toInt();
    slider->setValue(dpi_ / 25);
    updateLabel();
    connect( slider, &QSlider::valueChanged, this, &LaatuSlider::update);

    QHBoxLayout *leiska = new QHBoxLayout;
    leiska->addWidget(slider);
    leiska->addWidget(label);
    setLayout(leiska);
}

int LaatuSlider::value() const
{
    return dpi_;
}

void LaatuSlider::setValue(int dpi)
{
    slider->setValue(dpi / 25);
    update();
}

void LaatuSlider::update()
{
    int dpi = slider->value() * 25;
    if( dpi != dpi_) {
        dpi_ = dpi;
        kp()->settings()->setValue("AineistoLaatu", dpi);
        updateLabel();
    }
}

void LaatuSlider::updateLabel()
{
    label->setText(QString("%1 dpi").arg(dpi_));
}
