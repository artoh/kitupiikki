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
#include "tekstinaytin.h"
#include <QTextEdit>
#include <QPrinter>

Naytin::TekstiNaytin::TekstiNaytin(const QString &teksti, QObject *parent)
    : AbstraktiNaytin (parent),
      editori_( new QTextEdit )
{
    editori_->setReadOnly(true);
    editori_->setPlainText(teksti);
}

QWidget *Naytin::TekstiNaytin::widget()
{
    return editori_;
}

QByteArray Naytin::TekstiNaytin::data() const
{
    return editori_->toPlainText().toUtf8();
}

QString Naytin::TekstiNaytin::html() const
{
    return editori_->toHtml();
}

void Naytin::TekstiNaytin::paivita() const
{
    ;
}

void Naytin::TekstiNaytin::tulosta(QPrinter *printer) const
{
    editori_->print(printer);
}
