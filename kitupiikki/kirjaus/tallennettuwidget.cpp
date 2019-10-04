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
#include "tallennettuwidget.h"
#include "ui_tallennettuwidget.h"

#include "db/kirjanpito.h"

#include <QTimer>


TallennettuWidget::TallennettuWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TallennettuWidget),
    timer_(new QTimer(this))
{
    ui->setupUi(this);
    setVisible(false);

    setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    timer_->setSingleShot(true);
    connect( timer_, &QTimer::timeout, this, &TallennettuWidget::hide );
}

TallennettuWidget::~TallennettuWidget()
{
    delete ui;
}

void TallennettuWidget::nayta(int tunnus, const QDate &paiva, const QString &sarja)
{
    timer_->start(60000);

    ui->tunnisteellaLabel->setVisible( tunnus );
    ui->tunnisteLabel->setVisible( tunnus );
    ui->luonnosLabel->setVisible( !tunnus );

    if( tunnus ) {
        ui->tunnisteLabel->setText( QString("%3 %1 / %2")
                                .arg(tunnus)
                                .arg( kp()->tilikausiPaivalle(paiva).kausitunnus() )
                                .arg(sarja));
    }

    setVisible(true);
    raise();

}

void TallennettuWidget::piiloon()
{
    timer_->stop();
    hide();
}



void TallennettuWidget::mousePressEvent(QMouseEvent *event)
{
    piiloon();
    QWidget::mousePressEvent(event);
}
