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
#include "kpviiteedit.h"
#include "validator/viitevalidator.h"

#include <QPainter>

KpViiteEdit::KpViiteEdit(QWidget *parent) :
    QLineEdit(parent),
    validator_(new ViiteValidator(this))
{

}

ViiteNumero KpViiteEdit::viite()
{
    return ViiteNumero( text() );
}

void KpViiteEdit::setViite(const ViiteNumero &viite)
{
    setText( viite.valeilla() );
}

void KpViiteEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    QPainter painter(this);

    if( !text().isEmpty() ) {
        if( validator_->kelpaako(text())) {
            painter.drawPixmap( width()-20, height() / 2 - 8,  16, 16, QPixmap(":/pic/ok.png") );
        } else {
            painter.drawPixmap( width()-20, height() / 2 - 8,  16, 16, QPixmap(":/pic/varoitus.png") );
        }
    }
}
