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
    setInputMask("00.00.2\\000");
    setDate( kp()->paivamaara() );
    oletuspaiva_ = kp()->tilikaudet()->indeksiPaivalle(kp()->paivamaara()) > -1 ? kp()->paivamaara() : kp()->tilitpaatetty().addDays(1) ;
    setPlaceholderText( tr("pp.kk.vvvv","Päivämääräsyötön placeholder") );
    setStyleSheet("color: palette(text);");

    connect( kp(), &Kirjanpito::tietokantaVaihtui, this, &KpDateEdit::checkValidity);
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
    checkValidity();
}

void KpDateEdit::setEnabled(bool enabled)
{
    QLineEdit::setEnabled(enabled);
    checkValidity();
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
    if( dateInEditor_.isValid()
            && isEnabled()
            && ((dateInEditor_ < minimumDate() && minimumDate().isValid())
                || (dateInEditor_ > maximumDate() && maximumDate().isValid() && !property("SalliYlitys").toBool() ) ))
        setStyleSheet("color: red;");
    else {
        setStyleSheet("");
    }
}

bool KpDateEdit::isInvalid()
{
    if( dateInEditor_.isValid()
            && isEnabled()
            && ((dateInEditor_ < minimumDate() && minimumDate().isValid())
                || (dateInEditor_ > maximumDate() && maximumDate().isValid() && !property("SalliYlitys").toBool() ) ))
        return true;
    else {
        return false;
    }
}

void KpDateEdit::kalenteri()
{
    if(kalenteri_)
        kalenteri_->deleteLater();

    kalenteri_ = new QCalendarWidget(nullptr);

    kalenteri_->setWindowFlag(Qt::FramelessWindowHint);
    kalenteri_->setWindowFlag(Qt::Popup);
    kalenteri_->show();
    kalenteri_->move( mapToGlobal( QPoint(width() - kalenteri_->width(), height()  )));

    // Jotta kalenterista poistuttaessa kalenteri suljetaan:
    kalenteri_->installEventFilter(this);

    kalenteri_->setSelectedDate( date() );
    kalenteri_->setDateRange( minimumDate(), maximumDate() );
    kalenteri_->setGridVisible(true);

    connect( kalenteri_, &QCalendarWidget::clicked, this, &KpDateEdit::setDateFromPopUp);

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
        checkValidity();
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

        if( (date < minimumDate() && minimumDate().isValid()) || (date > maximumDate() && maximumDate().isValid() && !property("SalliYlitys").toBool())) {
            if( isEnabled())
                setStyleSheet("color: red;");
            date_ = QDate();            
        } else {
            setStyleSheet("");
            date_ = date;
        }
        emit dateChanged(date_);
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
    if( kalenteri_) {
        kalenteri_->close();
    }

}

void KpDateEdit::editMuuttui(const QString& uusi)
{
   // Hyödynnetään tietoa siitä, mikä luku on muuttunut

    int pp = uusi.mid(0,2).toInt();
    int kk = uusi.mid(3,2).toInt();
    int vv = uusi.mid(6,4).toInt();

   if( cursorPosition() == 1)
   {
       if( uusi.mid(0,1).toInt() > 3 ) {
           // 4 -> 04 jne.
           pp = uusi.mid(0,1).toInt();
           setCursorPosition(3);
       } else if (uusi.at(0)=='3' && uusi.mid(1) != '1')  {
           pp = 30;
       }
   }

   if( cursorPosition() == 3 )
   {
       if( uusi.at(0) == '3' && uusi.mid(1,1).toInt() > 1)
       {
           // 34 -> 03.04.
           pp=uusi.mid(0,1).toInt();
           kk=uusi.mid(1,1).toInt();
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
       if( kk < 20) {
           kk = 10;
       } else {
            kk = kk / 10;
            setCursorPosition( cursorPosition()+1 );
       }
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
    if( watched == kalenteri_ && (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Close ))
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
    } else if( event->key() == Qt::Key_Plus) {
        setDate( date().addDays(1));
    } else if( event->key() == Qt::Key_Minus) {
        setDate( date().addDays(-1));
    }
    else if( event->key() == Qt::Key_Space) {
        setCursorPosition(pos + 1);
        return;
    }

    if( text().isEmpty() && event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9 ) {
        QString oletusteksti = oletuspaiva_.toString("dd.MM.yyyy");
        if( maxDate_.isValid() && oletuspaiva_ > maxDate_)
            oletusteksti = maxDate_.toString("dd.MM.yyyy");
        else if( minDate_.isValid() && oletuspaiva_ < minDate_)
            oletusteksti = minDate_.toString("dd.MM.yyyy");
        setInputMask("00.00.2\\000");
        setText( QChar( event->key() - Qt::Key_0 + '0'  ) + oletusteksti.mid(1) );
        oletusteksti = QChar( event->key() - Qt::Key_0 + '0'  ) + oletusteksti.mid(1) ;
    }

    setCursorPosition(pos);

    QLineEdit::keyPressEvent(event);
    editMuuttui( text() );
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

KpDateEditDelegaatille::KpDateEditDelegaatille(QWidget *parent)
    : KpDateEdit(parent)
{

}

void KpDateEditDelegaatille::keyPressEvent(QKeyEvent *event)
{
    if( event->key() == Qt::Key_Up || event->key() == Qt::Key_Down )
        return QLineEdit::keyPressEvent(event);
    else
        KpDateEdit::keyPressEvent(event);
}
