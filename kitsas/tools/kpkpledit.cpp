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
#include "kpkpledit.h"

#include <QKeyEvent>

KpKplEdit::KpKplEdit(QWidget *parent) :
    QLineEdit(parent)
{

}

void KpKplEdit::setText(const QString &value)
{
    QString teksti = value;
    teksti.replace('.',',');
    QLineEdit::setText(teksti);
}

QString KpKplEdit::text() const
{
    QString arvo = QLineEdit::text();
    arvo.replace(',','.');
    return arvo;

}

double KpKplEdit::kpl() const
{
    return text().toDouble();
}

void KpKplEdit::keyPressEvent(QKeyEvent *event)
{
    const QString teksti = QLineEdit::text();
    if( event->text().isEmpty()
            || event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete
            || event->text().at(0).isDigit()) {
        QLineEdit::keyPressEvent(event);
    } else if ( (event->text() == "." || event->text() == ",") &&
               !teksti.contains(',') ) {
        QKeyEvent piste(QEvent::KeyPress, Qt::Key_Period, Qt::NoModifier, ",");
        QLineEdit::keyPressEvent(&piste);
    } else if( event->text() == "-" || event->text() == "+") {
        bool miinus = teksti.startsWith("-");
        const int kursorinpaikka = cursorPosition();

        if( event->key() == '-' && !miinus) {
            QLineEdit::setText("-" + teksti);
            setCursorPosition( kursorinpaikka + 1);
        } else if( event->key() == '+' && miinus) {
            QLineEdit::setText( teksti.mid(1) );
            setCursorPosition( kursorinpaikka - 1);
        }
    }
}
