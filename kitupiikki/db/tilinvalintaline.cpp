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

    QCompleter *taydennin = new QCompleter() ;
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

void KantaTilinvalintaLine::valitseTiliIdlla(int tiliId)
{
    Tili tili = kp()->tilit()->tiliIdlla(tiliId);
    valitseTili(tili);
}

void KantaTilinvalintaLine::valitseTili(const Tili& tili)
{
    if( tili.id())
        setText( tr("%1 %2").arg(tili.numero()).arg(tili.nimi()));
    else
        setText("");

}

void KantaTilinvalintaLine::suodataTyypilla(const QString &regexp)
{
    proxyTyyppi_->setFilterRegExp(regexp);
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
        qobject_cast<QWidget*>(parent())->setFocus();
    }
    else
        QLineEdit::keyPressEvent(event);
}



TilinvalintaLine::TilinvalintaLine(QWidget *parent)
    : KantaTilinvalintaLine(parent), model_( nullptr )
{

}

void TilinvalintaLine::asetaModel(TiliModel *model)
{
    proxyTyyppi_->setSourceModel( model );
    model_ = model;
}

void TilinvalintaLine::keyPressEvent(QKeyEvent *event)
{

    if( (!event->text().isEmpty() && event->text().at(0).isLetter())
            || event->key() == Qt::Key_Space)
    {
        Tili valittu;

        if( event->key() == Qt::Key_Space)
            valittu = TilinValintaDialogi::valitseTili(QString(), proxyTyyppi_->filterRegExp().pattern(), model_ );
        else
            valittu = TilinValintaDialogi::valitseTili( event->text(), proxyTyyppi_->filterRegExp().pattern(), model_ );
        if( valittu.id())
        {
            valitseTili( valittu);
            emit editingFinished();
        }

    }
    else
        KantaTilinvalintaLine::keyPressEvent(event);
}
