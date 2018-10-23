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

#include "kpdateedit.h"
#include "db/kirjanpito.h"



KpDateEdit::KpDateEdit(QWidget *parent) :
    QLineEdit(parent),
    kalenteri_(nullptr),
    popupKaytossa_(false),
    suljettu_(0)
{
    setDateRange( kp()->tilitpaatetty().addDays(1) , kp()->tilikaudet()->kirjanpitoLoppuu()  );

    setInputMask("00.00.2\\000");
    setDate( kp()->paivamaara() );

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

void KpDateEdit::setDateRange(const QDate &min, const QDate &max)
{
    minDate_ = min;
    maxDate_ = max;
}

void KpDateEdit::setCalendarPopup(bool enable)
{
    popupKaytossa_ = enable;
}

void KpDateEdit::kalenteri()
{
    if(kalenteri_)
        kalenteri_->deleteLater();

    kalenteri_ = new QCalendarWidget(0);

    kalenteri_->setWindowFlag(Qt::FramelessWindowHint);
    kalenteri_->show();
    kalenteri_->move( mapToGlobal( QPoint(0, height()  )));

    // Jotta kalenterista poistuttaessa kalenteri suljetaan:
    kalenteri_->installEventFilter(this);

    kalenteri_->setSelectedDate( date() );
    kalenteri_->setDateRange( minimumDate(), maximumDate() );
    kalenteri_->setGridVisible(true);
    kalenteri_->resize(300,200);

    connect( kalenteri_, SIGNAL(clicked(QDate)), this, SLOT(setDate(QDate)));

}

void KpDateEdit::setDate(QDate date)
{
    if( date.isValid())
    {
        // Jos päivämäärä on minimiä pienempi, laitetaan ensin vuosi lisää. Tämä siksi, että tavanomaisessa
        // tilanteessa muokataan vuoden viimeistä päivämäärää eteenpäin

        if( date < minimumDate() )
            date = date.addYears(1);

        if( date < minimumDate() )
            date = minimumDate();
        else if( date > maximumDate())
            date = maximumDate();

        date_ = date;
        int pos = cursorPosition();
        setText( date.toString("dd.MM.yyyy") );
        setCursorPosition(pos);
        emit dateChanged( date );
    }

    // Kalenteri piilotetaan
    if( kalenteri_)
    {
        kalenteri_->deleteLater();
        kalenteri_ = 0;
    }

}

void KpDateEdit::editMuuttui(const QString& uusi)
{
   int pp = uusi.midRef(0,2).toInt();
   int kk = uusi.midRef(3,2).toInt();
   int vv = uusi.midRef(6,4).toInt();

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
   // 32 -> 30
   else if( pp > 31)
       pp = 30;

   if( kk > 20)
   {
        kk = kk / 10;
        setCursorPosition( cursorPosition()+1 );
   }
   if( kk > 12)
   {
       kk = 10;
   }

    if( !QDate(vv,kk,pp).isValid() )
        kk = kk + 1;


    QDate pvm(vv,kk,pp);

    if( pvm.isValid())
    {
        setDate(pvm);
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

    if( popupKaytossa_)
        painter.drawPixmap( width() - 20, height() / 2 - 8,  16, 16, QPixmap(":/pic/kalenteri16.png") );
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

    if( event->pos().x() > width() - 22 && popupKaytossa_ && !kalenteri_ &&
            QDateTime::currentMSecsSinceEpoch() - suljettu_ > 10)
    {
            kalenteri();
    }
    else
        QLineEdit::mousePressEvent(event);
}

void KpDateEdit::mouseMoveEvent(QMouseEvent *event)
{
    if( event->pos().x() > width() - 22 && popupKaytossa_ )
        setCursor( Qt::ArrowCursor );
    else
        setCursor( Qt::IBeamCursor);

    QLineEdit::mouseMoveEvent( event);
}
