/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include "tilinvalintaline.h"
#include "tilinvalintadialogi.h"

#include <QCompleter>

#include <QDebug>
#include <QKeyEvent>
#include <QPainter>

KantaTilinvalintaLine::KantaTilinvalintaLine(QWidget *parent)
    : QLineEdit(parent)
{


    proxyTyyppi_ = new QSortFilterProxyModel(parent);
    proxyTyyppi_->setSourceModel( kp()->tilit());
    proxyTyyppi_->setFilterRole(TiliModel::TyyppiRooli);
    proxyTyyppi_->setFilterRegExp("[ABCD].*");

    proxyTila_ = new QSortFilterProxyModel(parent);
    proxyTila_->setSourceModel(proxyTyyppi_);
    proxyTila_->setFilterRole(TiliModel::TilaRooli);
    proxyTila_->setFilterRegExp("[12]");

    QCompleter *taydennin = new QCompleter(this) ;
    taydennin->setCompletionColumn( TiliModel::NRONIMI);
    taydennin->setModel( proxyTila_ );

    taydennin->setCompletionMode( QCompleter::UnfilteredPopupCompletion);
    setCompleter(taydennin);
}

int KantaTilinvalintaLine::valittuTilinumero() const
{
    return valittuTili().numero();
}

void KantaTilinvalintaLine::valitseTiliNumerolla(int tilinumero)
{
    if( tilinumero )
    {
        Tili tili = kp()->tilit()->tiliNumerolla(tilinumero);
        valitseTili(tili);
    }

}

void KantaTilinvalintaLine::valitseTili(const Tili& tili)
{
    if( tili.onkoValidi())
    {
        setText( QString("%1 %2").arg(tili.numero()).arg(tili.nimi()));
        setCursorPosition(0);
    }
    else
        setText("");

}

void KantaTilinvalintaLine::valitseTili(const Tili *tili)
{
    if( tili)
        valitseTiliNumerolla(tili->numero());
    else
        setText("");
}

void KantaTilinvalintaLine::suodataTyypilla(const QString &regexp)
{
    proxyTyyppi_->setFilterRole(TiliModel::TyyppiRooli);
    proxyTyyppi_->setFilterRegExp(regexp);
}

void KantaTilinvalintaLine::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    QPainter painter(this);
    painter.drawPixmap( width() - 20, height() / 2 - 8,  16, 16, QPixmap(":/pic/etsitili.png") );

}

void KantaTilinvalintaLine::mouseMoveEvent(QMouseEvent *event)
{
    if( event->pos().x() > width() - 22 )
        setCursor( Qt::ArrowCursor );
    else
        setCursor( Qt::IBeamCursor);

    QLineEdit::mouseMoveEvent( event);
}

Tili KantaTilinvalintaLine::valittuTili() const
{
    QString sana = text().left( text().indexOf(' '));
    if( sana.isEmpty() || !sana.at(0).isDigit() )
        return Tili();

    for( int i=0; i < kp()->tilit()->rowCount( QModelIndex()); i++)
    {
        Tili tili = kp()->tilit()->tiliIndeksilla(i);
        if( !tili.otsikkotaso())
        {
            QString nroTxt = QString::number( tili.numero() );
            if( nroTxt.startsWith(sana))
            {
                return tili;
            }
        }
    }
    return Tili();
}

Tili *KantaTilinvalintaLine::tili() const
{
    return kp()->tilit()->tili( valittuTilinumero() );
}



TilinvalintaLineDelegaatille::TilinvalintaLineDelegaatille(QWidget *parent) :
    KantaTilinvalintaLine(parent)
{

}

void TilinvalintaLineDelegaatille::keyPressEvent(QKeyEvent *event)
{
    if( (!event->text().isEmpty() && event->text().at(0).isLetter())
            || event->key() == Qt::Key_Space)
    {
        alku_ = event->text();

        QString sana = text().left( text().indexOf(' '));
        if( !sana.isEmpty() && sana.at(0).isDigit() && ( alku_.isEmpty() || !alku_.at(0).isNumber()) )
            alku_ = "*" + sana;   // Tähän mekanismi saada dialogi, jossa valittuna (esim * alkuun)

        if(qobject_cast<QWidget*>(parent()))
            qobject_cast<QWidget*>(parent())->setFocus();
    }
    else
    {
        if( !event->text().isEmpty() && event->text().at(0).isNumber() && cursorPosition() == 0 )
            clear();
        QLineEdit::keyPressEvent(event);
    }
}



TilinvalintaLine::TilinvalintaLine(QWidget *parent)
    : KantaTilinvalintaLine(parent), model_( nullptr )
{

}

void TilinvalintaLine::asetaModel(TiliModel *model)
{
    model_ = model;
    proxyTyyppi_->setSourceModel( model );
    proxyTila_->setSortRole(TiliModel::LajitteluRooli);
    proxyTila_->sort(TiliModel::NUMERO);
}

void TilinvalintaLineDelegaatille::mousePressEvent(QMouseEvent *event)
{
    if( event->pos().x() > width() - 22)
    {
        QString sana = text().left( text().indexOf(' '));
        if( !sana.isEmpty() && sana.at(0).isDigit() )
            alku_ = "*" + sana;   // Tähän mekanismi saada dialogi, jossa valittuna (esim * alkuun)
        else
            alku_ = " ";

        if(qobject_cast<QWidget*>(parent()))
            qobject_cast<QWidget*>(parent())->setFocus();
    }
}


void TilinvalintaLine::keyPressEvent(QKeyEvent *event)
{

    if( (!event->text().isEmpty() && event->text().at(0).isLetter())
            || event->key() == Qt::Key_Space)
    {
        Tili valittu;

        if( event->key() == Qt::Key_Space) {
            QString alku;
            QString sana = text().left( text().indexOf(' '));
            if( !sana.isEmpty() && sana.at(0).isDigit() )
                alku = "*" + sana;

            valittu = TilinValintaDialogi::valitseTili(alku, proxyTyyppi_->filterRegExp().pattern(), model_ );
        } else
            valittu = TilinValintaDialogi::valitseTili( event->text(), proxyTyyppi_->filterRegExp().pattern(), model_ );
        if( valittu.numero())
        {
            valitseTiliNumerolla( valittu.numero() );
            emit editingFinished();
        }

    }
    else
    {
        if( !event->text().isEmpty() && event->text().at(0).isNumber() && cursorPosition() == 0 )
            clear();
        KantaTilinvalintaLine::keyPressEvent(event);
    }

}

void TilinvalintaLine::mousePressEvent(QMouseEvent *event)
{
    if( event->pos().x() > width() - 22)
    {
        QString alku;
        QString sana = text().left( text().indexOf(' '));

        if( !sana.isEmpty() && sana.at(0).isDigit() )
            alku = "*" + sana;   // Tähän mekanismi saada dialogi, jossa valittuna (esim * alkuun)

        Tili valittu = TilinValintaDialogi::valitseTili( alku, proxyTyyppi_->filterRegExp().pattern(), kp()->tilit() );
        if( valittu.onkoValidi())
        {
            valitseTili( valittu);
        }
    }
}
