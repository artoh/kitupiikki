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
#include "viennitview.h"

#include "model/tositeviennit.h"
#include "db/kirjanpito.h"

#include "tilidelegaatti.h"
#include "eurodelegaatti.h"
#include "pvmdelegaatti.h"
#include "kohdennusdelegaatti.h"
#include "model/tositeviennit.h"

#include <QHeaderView>
#include <QSettings>
#include <QKeyEvent>

ViennitView::ViennitView(QWidget *parent)
    : QTableView (parent)
{
    setItemDelegateForColumn( TositeViennit::TILI, new TiliDelegaatti( ) );
    setItemDelegateForColumn( TositeViennit::DEBET, new EuroDelegaatti);
    setItemDelegateForColumn( TositeViennit::KREDIT, new EuroDelegaatti);
    setItemDelegateForColumn( TositeViennit::KOHDENNUS, new KohdennusDelegaatti);

    horizontalHeader()->setStretchLastSection(true);


    // Ladataan leveyslista
    QStringList leveysLista = kp()->settings()->value("KirjausWgRuudukko").toStringList();
    for(int i=0; i < leveysLista.count()-1; i++)
        horizontalHeader()->resizeSection(i, leveysLista.at(i).toInt());
}

void ViennitView::seuraavaSarake()
{
    const QModelIndex index = currentIndex();
    if( index.row() == model()->rowCount() -1 &&
        index.column() == model()->columnCount() -1)
    {
        // Lisätään uusi rivi
        TositeViennit *vientiModel = qobject_cast<TositeViennit*>( model() );
        vientiModel->lisaaVienti( model()->rowCount() );
        setCurrentIndex( vientiModel->index( model()->rowCount() - 1, TositeViennit::TILI ) );
    }
    else if( index.column() == model()->columnCount() - 1 )
    {
        setCurrentIndex( model()->index(  index.row()+1 , TositeViennit::PVM ) );
    }
    else if( index.column() == TositeViennit::TILI)
    {
        // Jos tili on tulotili tai vastattavaa, hypätään suoraan Kreditiin
        Tili tili = kp()->tilit()->tiliNumerolla( currentIndex().data(TositeViennit::TiliNumeroRooli).toInt() );
        if( tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::VASTATTAVAA))
            setCurrentIndex( model()->index( index.row(), TositeViennit::KREDIT ) );
        else
            setCurrentIndex( model()->index( index.row(), TositeViennit::DEBET ) );
    }
    else
    {
        // Jos ALV-sarake on piilotettu, niin sen yli hypätään
        if( isColumnHidden( index.column() + 1) )
            setCurrentIndex( model()->index( index.row(), index.column() + 2 ) );
        else
            setCurrentIndex( model()->index( index.row(), index.column() + 1 ) );

    }

    // Vielä hypyt: Täytetystä debetistä kreditin yli
    // Kohdennuksen yli, jos kohdennuksia ei käytössä

}

void ViennitView::keyPressEvent(QKeyEvent *event)
{
    if( event->modifiers() == Qt::KeyboardModifier::NoModifier &&
        event->key() == Qt::Key_Tab)
    {
        seuraavaSarake();
    }
    else
    {
        QTableView::keyPressEvent( event );
    }
}

void ViennitView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);

    if( hint == QAbstractItemDelegate::EditNextItem)
        seuraavaSarake();

    // @TODO: edellinen sarake
}
