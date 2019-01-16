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

#include "db/vientimodel.h"
#include "db/kirjanpito.h"

#include "tilidelegaatti.h"
#include "eurodelegaatti.h"
#include "pvmdelegaatti.h"
#include "kohdennusdelegaatti.h"



#include <QHeaderView>
#include <QSettings>

ViennitView::ViennitView(QWidget *parent)
    : QTableView (parent)
{
    setItemDelegateForColumn( VientiModel::TILI, new TiliDelegaatti( ) );
    setItemDelegateForColumn( VientiModel::DEBET, new EuroDelegaatti);
    setItemDelegateForColumn( VientiModel::KREDIT, new EuroDelegaatti);
    setItemDelegateForColumn( VientiModel::KOHDENNUS, new KohdennusDelegaatti);

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
        VientiModel *vientiModel = qobject_cast<VientiModel*>( model() );
        vientiModel->lisaaVienti();
        setCurrentIndex( vientiModel->index( model()->rowCount() - 1, VientiModel::TILI ) );
    }
    else if( index.column() == model()->columnCount() - 1 )
    {
        setCurrentIndex( model()->index(  index.row()+1 , VientiModel::PVM ) );
    }
    else if( index.column() == VientiModel::TILI)
    {
        // Jos tili on tulotili tai vastattavaa, hypätään suoraan Kreditiin
        Tili tili = kp()->tilit()->tiliIdlla( currentIndex().data(VientiModel::TiliIdRooli).toInt() );
        if( tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::VASTATTAVAA))
            setCurrentIndex( model()->index( index.row(), VientiModel::KREDIT ) );
        else
            setCurrentIndex( model()->index( index.row(), VientiModel::DEBET ) );
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
