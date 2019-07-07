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
#include "kpeuroedit.h"

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QKeyEvent>
#include <QDebug>

KpEuroEdit::KpEuroEdit(QWidget *parent) :
    QLineEdit (parent), cents_(0l)
{
    setAlignment(Qt::AlignRight);
    setText("0,00 €");

    QRegularExpression re("(−)?[\\s\\d]{0,16},\\d{0,2}( €)?");
    re.setPatternOptions(QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(re,this);
    setValidator(validator);
    setMaxLength(20);

    connect(this, &KpEuroEdit::textChanged, this, &KpEuroEdit::edited);
}

void KpEuroEdit::setCents(qlonglong cents)
{
    cents_ = cents;
    setText(QString("%L1 €").arg( cents_ / 100.0 ,0,'f',2));
    setClearButtonEnabled( cents );
}

void KpEuroEdit::setValue(double euros)
{
    setCents(qRound( euros * 100.0 ));
}

void KpEuroEdit::edited(const QString &newtext)
{
    QString teksti = newtext;
    int pilkunpaikka = teksti.indexOf(',');
    if( pilkunpaikka == -1)
        pilkunpaikka = teksti.lastIndexOf(QRegularExpression("\\d"))+1;
    int kursorinpaikka = cursorPosition();
    int suhteellinen = pilkunpaikka - kursorinpaikka;
    int oikaistaanvalit = 0;
    for(int i=kursorinpaikka; i<pilkunpaikka;i++)
        if( teksti.at(i).isSpace())
            oikaistaanvalit++;


    bool miinus = teksti.contains("−");
    QString eurosa = teksti.left(pilkunpaikka);
    QString sentosa = teksti.mid(pilkunpaikka);
    eurosa.remove(QRegularExpression("\\D"));
    sentosa.remove(QRegularExpression("\\D"));

    int sentit = pilkunpaikka > -1 ? sentosa.toInt() : 0;

    if( sentit < 10 && sentosa.length() == 1)
        sentit *= 10;

    cents_ = eurosa.toLongLong() * 100 + sentit;
    if(miinus && !cents_)
        return; // -0
    if(miinus)
        cents_ *= -1;

    setClearButtonEnabled(cents_);

    setText(QString("%L1 €").arg( cents_ / 100.0 ,0,'f',2));

    int uusipilkunpaikka = text().indexOf(',');
    if( uusipilkunpaikka == -1)
        uusipilkunpaikka = text().lastIndexOf(QRegularExpression("\\d"))+1;

    int laskennallisetvalit = (suhteellinen - 1) / 3;
    if( suhteellinen < 0)
        setCursorPosition( pilkunpaikka - suhteellinen );
    else {

        int uusipaikka = uusipilkunpaikka -  suhteellinen + oikaistaanvalit - laskennallisetvalit;
        setCursorPosition(uusipaikka);
    }

}

void KpEuroEdit::keyPressEvent(QKeyEvent *event)
{
    int kursorinpaikka = cursorPosition();
    int pilkunpaikka = text().indexOf(',');

    if( event->key() == Qt::Key_Comma)
    {
        int pilkunpaikka = text().indexOf(',');
        if( pilkunpaikka > -1)
        {
            setCursorPosition(pilkunpaikka+1);
            return;
        }
    }
    else if( event->key() == Qt::Key_Minus && !hasSelectedText())
    {
        if( text().startsWith("−"))
        {
            setText( text().mid(1) );
            setCursorPosition(kursorinpaikka-1);
        }
        else
        {
            setText( "−" + text());
            setCursorPosition(kursorinpaikka+1);
        }
        return;
    }
    else if( event->key() == Qt::Key_Left && kursorinpaikka > 1 && text().at(kursorinpaikka-1).isSpace() )
    {
        cursorBackward(false,1);
        return;
    }
    else if( event->key() == Qt::Key_Right && kursorinpaikka < text().length()-1 && text().at(kursorinpaikka).isSpace() )
    {
        cursorForward(false,1);
        return;
    }
    else if( event->key() == Qt::Key_Backspace && kursorinpaikka == pilkunpaikka+1)
    {
        cursorBackward(false,1);
    }
    else if( event->key() == Qt::Key_Delete && kursorinpaikka == pilkunpaikka)
    {
        cursorForward(false,1);
    }
    else if( !event->text().isEmpty() && event->text().at(0).isDigit())
    {
        if( pilkunpaikka > -1 && kursorinpaikka > pilkunpaikka &&
                kursorinpaikka < text().length())
        {
            if( text().at(kursorinpaikka).isDigit() )
            {
                setText( text().left(kursorinpaikka) + event->text() +
                         text().mid(kursorinpaikka+1));
                setCursorPosition(kursorinpaikka+1);
                return;
            }
        }
        else if( !kursorinpaikka  && text() == "0,00 €" ) {
            setText( event->text() + ",00 €");
            setCursorPosition(1);
            return;
        }
    }

    QLineEdit::keyPressEvent(event);
}



