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

#include <QDebug>

#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QCalendarWidget>
#include <QDate>
#include <QEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QApplication>

#include "kpdateedit.h"
#include "db/kirjanpito.h"



KpDateEdit::KpDateEdit(QWidget *parent) :
    QLineEdit(parent),
    kalenteri_(nullptr),
    nullMahdollinen_(false),
    suljettu_(0)
{
    setDateRange( kp()->tilitpaatetty().addDays(1) , kp()->tilikaudet()->kirjanpitoLoppuu()  );

    setInputMask("00.00.2\\000");
    setDate( kp()->paivamaara() );
    oletuspaiva_ = date();
    setPlaceholderText( tr("pp.kk.vvvv") );

    connect( this, SIGNAL(textEdited(QString)), this, SLOT(editMuuttui(QString)));
}

KpDateEdit::~KpDateEdit()
{
    if( kalenteri_)
        kalenteri_->deleteLater();
}

QSize KpDateEdit::sizeHint() const
{
    return QSize( 200,
                  QLineEdit::sizeHint().height());

}

QDate KpDateEdit::date() const
{
    return date_;
}

void KpDateEdit::setDateRange(const QDate &min, const QDate &max)
{
    minDate_ = min;
    maxDate_ = max;
}

void KpDateEdit::setNullable(bool enable)
{
    nullMahdollinen_ = enable;
}

void KpDateEdit::setNull()
{
    nullMahdollinen_ = true;
    if( !oletuspaiva_.isValid() )
        oletuspaiva_ = kp()->paivamaara();
    if( oletuspaiva_.isValid())
        setDate( QDate());
}

void KpDateEdit::setDefaultDate(const QDate &date)
{
    oletuspaiva_ = date;
    setNullable( date.isValid() );
}

void KpDateEdit::checkValidity()
{
    if( (date() < minimumDate() && minimumDate().isValid()) || (date() > maximumDate() && maximumDate().isValid()) )
        setStyleSheet("color: red;");
    else {
        setStyleSheet("");
    }
}

void KpDateEdit::kalenteri()
{
    if(kalenteri_)
        kalenteri_->deleteLater();

    kalenteri_ = new QCalendarWidget(nullptr);

    kalenteri_->setWindowFlag(Qt::FramelessWindowHint);
    kalenteri_->show();
    kalenteri_->move( mapToGlobal( QPoint(0, height()  )));

    // Jotta kalenterista poistuttaessa kalenteri suljetaan:
    kalenteri_->installEventFilter(this);

    kalenteri_->setSelectedDate( date() );
    kalenteri_->setDateRange( minimumDate(), maximumDate() );
    kalenteri_->setGridVisible(true);

    connect( kalenteri_, SIGNAL(clicked(QDate)), this, SLOT( setDateFromPopUp(QDate)));

}

void KpDateEdit::setDate(QDate date)
{
    if( date.isValid()) {
        date_ = date;
        dateInEditor_ = date;

        int pos =  hasFocus() ? cursorPosition() : -1;
        setText( date.toString("dd.MM.yyyy") );
        if( pos > -1)
            setCursorPosition(pos);
    }
    else if( isNullable() )
    {
        if( date_.isValid())
            oletuspaiva_ = date_;

        date_ = QDate();
        dateInEditor_ = QDate();

        setInputMask(QString());
        setText(QString());
        setStyleSheet("");
    }

    emit dateChanged( date );

}

void KpDateEdit::setDateInternal(const QDate &date)
{
    if( date.isValid())
    {

        int pos = cursorPosition();
        if( dateInEditor_.isNull() )
        {
            setInputMask("00.00.2\\000");
        }

        dateInEditor_ = date;
        setText( date.toString("dd.MM.yyyy") );

        setCursorPosition(pos);

        if( (date < minimumDate() && minimumDate().isValid()) || (date > maximumDate() && maximumDate().isValid())) {
            setStyleSheet("color: red;");
            date_ = QDate();
            emit dateChanged(date_);
        } else {
            setStyleSheet("");
            setDate( date );
        }
    }

    // Kalenteri piilotetaan
    if( kalenteri_)
    {
        kalenteri_->deleteLater();
        kalenteri_ = nullptr;
    }
}

void KpDateEdit::setDateFromPopUp(const QDate &date)
{
    setDate( date );
    QKeyEvent *event = new QKeyEvent( QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    qApp->postEvent( parent(), event );

}

void KpDateEdit::editMuuttui(const QString& uusi)
{
    if( isNullable() && !date_.isValid() )
    {
        if( !uusi.isEmpty() && uusi.at(0).isDigit())
        {
            setDateInternal( oletuspaiva_ );
            editMuuttui( uusi + text().mid(1));            
        }
        return;
    }

   // Hyödynnetään tietoa siitä, mikä luku on muuttunut

    int pp = uusi.midRef(0,2).toInt();
    int kk = uusi.midRef(3,2).toInt();
    int vv = uusi.midRef(6,4).toInt();

   if( cursorPosition() == 1)
   {
       if( uusi.midRef(0,1).toInt() > 3 ) {
           // 4 -> 04 jne.
           pp = uusi.midRef(0,1).toInt();
           setCursorPosition(3);

       } else if( uusi.at(1) == '9' && QString("123").contains( uusi.at(0) ) ) {
           // Päivämäärän korottaminen
           pp = uusi.mid(0,1).toInt() * 10;
       }
   }

   if( cursorPosition() == 3 )
   {
       if( uusi.at(0) == '3' && uusi.midRef(1,1).toInt() > 1)
       {
           // 34 -> 03.04.
           pp=uusi.midRef(0,1).toInt();
           kk=uusi.midRef(1,1).toInt();
           setCursorPosition(8);
       }
   }



   // päivä tai kk ei voi olla 0
   if( !pp)
       pp = 1;
   if( !kk)
       kk = 1;

   // 40 -> 04 jne.
   if( pp > 40)
   {
        pp = pp / 10;
        setCursorPosition( cursorPosition()+1 );
   }
   else if( pp > 31)
   {
       pp = 31;
   }

   if( kk > 12)
   {
        kk = kk / 10;
        setCursorPosition( cursorPosition()+1 );
   }

    if( !QDate(vv,kk,pp).isValid() )
        kk = kk + 1;


    QDate pvm(vv,kk,pp);

    if( pvm.isValid())
    {
        setDateInternal(pvm);
    }

}



bool KpDateEdit::eventFilter(QObject *watched, QEvent *event)
{
    // Jotta kalenterista fokuksen kadotessa kalenteri katoaisi
    if( watched == kalenteri_ && event->type() == QEvent::WindowDeactivate )
    {
        kalenteri_->deleteLater();
        kalenteri_ = nullptr;
        suljettu_ = QDateTime::currentMSecsSinceEpoch();
    }

    return QWidget::eventFilter(watched, event);
}

void KpDateEdit::keyPressEvent(QKeyEvent *event)
{
    int pos = cursorPosition();

    if( event->key() == Qt::Key_Up)
    {
        if( pos < 2)
            setDate( date().addDays(1) );
        else if( 2 < pos && pos < 5)
            setDate( date().addMonths(1));
        else if( 5 < pos )
            setDate( date().addYears(1));
    }
    else if( event->key() == Qt::Key_Down)
    {
        if( pos < 2)
            setDate( date().addDays(-1));
        else if( 2 < pos && pos < 5)
            setDate( date().addMonths(-1));
        else if( 5 < pos)
            setDate( date().addYears(-1));
    }

    setCursorPosition(pos);

    QLineEdit::keyPressEvent(event);
}

void KpDateEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    QPainter painter(this);

    painter.drawPixmap( width()-20, height() / 2 - 8,  16, 16, QPixmap(":/pic/kalenteri16.png") );
    if( nullMahdollinen_ )
    {
        painter.drawPixmap( width()-40, height() / 2 - 8,  16, 16, QPixmap(":/pic/clear16.png") );
    }

}

void KpDateEdit::focusInEvent(QFocusEvent * event)
{
    setCursorPosition(0);
    QLineEdit::focusInEvent(event);
}

void KpDateEdit::mousePressEvent(QMouseEvent *event)
{
    // suljettu_ -ehdolla varmistetaan, ettei kyse ole saman napsautuksen
    // aiheuttamasta eventistä, jolla yritettiin sulkea ikkuna

    if( event->pos().x() > width() - 20 && !kalenteri_ &&
            QDateTime::currentMSecsSinceEpoch() - suljettu_ > 10)
    {
            kalenteri();
    }
    else if( event->pos().x() < width()-20 && event->pos().x() > width()-40 && isNullable() )
    {
        // Arvoksi tulee null
        setDate( QDate() );
    }

    else
        QLineEdit::mousePressEvent(event);
}

void KpDateEdit::mouseMoveEvent(QMouseEvent *event)
{
    if( event->pos().x() > width() - 20 || ( event->pos().x() > width() - 40 && nullMahdollinen_ ))
        setCursor( Qt::ArrowCursor );
    else
        setCursor( Qt::IBeamCursor);

    QLineEdit::mouseMoveEvent( event);
}
