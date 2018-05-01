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

#include "kpdateedit.h"
#include "db/kirjanpito.h"



KpDateEdit::KpDateEdit(QWidget *parent) :
    QWidget(parent),
    kalenteri_(0)
{
    editori_ = new QLineEdit(this);
    nappi_ = new QPushButton(this);
    nappi_->setIcon(QIcon(":/pic/kalenteri16.png"));

    setDateRange( kp()->tilitpaatetty().addDays(1) , kp()->tilikaudet()->kirjanpitoLoppuu()  );

    editori_->setInputMask("00.00.2\\000");
    setDate( kp()->paivamaara() );

    resizeEvent();

    connect( editori_, SIGNAL(textEdited(QString)), this, SLOT(editMuuttui(QString)));
    connect( nappi_, SIGNAL(clicked(bool)), this, SLOT(kalenteri()) );
    connect( editori_, SIGNAL(editingFinished()), this, SIGNAL(editingFinished()));
}

KpDateEdit::~KpDateEdit()
{
    if( kalenteri_)
        kalenteri_->deleteLater();
}

QSize KpDateEdit::sizeHint() const
{
    return QSize( editori_->sizeHint().width() + 20,
                  editori_->sizeHint().height());
}

void KpDateEdit::setDateRange(const QDate &min, const QDate &max)
{
    minDate_ = min;
    maxDate_ = max;
}

void KpDateEdit::kalenteri()
{
    if(kalenteri_)
        kalenteri_->deleteLater();

    kalenteri_ = new QCalendarWidget(0);

    kalenteri_->setWindowFlag(Qt::FramelessWindowHint);
    kalenteri_->show();
    kalenteri_->move( mapToGlobal( QPoint(0, editori_->height()  )));

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
        if( date < minimumDate() )
            date = minimumDate();
        else if( date > maximumDate())
            date = maximumDate();

        date_ = date;
        editori_->setText( date.toString("dd.MM.yyyy") );
        emit dateChanged( date );
    }

    // Kalenteri piilotetaan
    if( kalenteri_)
    {
        kalenteri_->deleteLater();
        kalenteri_ = 0;
    }

}

void KpDateEdit::editMuuttui(QString uusi)
{
   int pp = uusi.mid(0,2).toInt();
   int kk = uusi.mid(3,2).toInt();
   int vv = uusi.mid(6,4).toInt();

   // päivä tai kk ei voi olla 0
   if( !pp)
       pp = 1;
   if( !kk)
       kk = 1;

   // 40 -> 04 jne.
   if( pp > 40)
   {
        pp = pp / 10;
        editori_->setCursorPosition( editori_->cursorPosition()+1 );
   }
   // 32 -> 30
   else if( pp > 31)
       pp = 30;

   if( kk > 20)
   {
        kk = kk / 10;
        editori_->setCursorPosition( editori_->cursorPosition()+1 );
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
        int pos = editori_->cursorPosition();
        setDate(pvm);
        editori_->setCursorPosition(pos);
    }

}

void KpDateEdit::resizeEvent()
{
    editori_->setGeometry(0,0,width()-20, height());
    nappi_->setGeometry(width()-20, 0, 20, height());
}

bool KpDateEdit::eventFilter(QObject *watched, QEvent *event)
{
    // Jotta kalenterista fokuksen kadotessa kalenteri katoaisi
    if( watched == kalenteri_ && event->type() == QEvent::WindowDeactivate )
    {
        kalenteri_->deleteLater();
        kalenteri_ = 0;
    }

    return QWidget::eventFilter(watched, event);
}

void KpDateEdit::keyPressEvent(QKeyEvent *event)
{
    int pos = editori_->cursorPosition();

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

    editori_->setCursorPosition(pos);

    QWidget::keyPressEvent(event);
}
