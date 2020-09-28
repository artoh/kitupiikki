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
#include "model/tosite.h"

#include <QHeaderView>
#include <QSettings>
#include <QKeyEvent>

ViennitView::ViennitView(QWidget *parent)
    : QTableView (parent)
{
    setItemDelegateForColumn( TositeViennit::TILI, new TiliDelegaatti(this ) );
    setItemDelegateForColumn( TositeViennit::DEBET, new EuroDelegaatti(this));
    setItemDelegateForColumn( TositeViennit::KREDIT, new EuroDelegaatti(this));
    setItemDelegateForColumn( TositeViennit::KOHDENNUS, new KohdennusDelegaatti(this));

    setEditTriggers( QTableView::AllEditTriggers );

    horizontalHeader()->setStretchLastSection(true);

    setFocusPolicy(Qt::StrongFocus);

    // Ladataan leveyslista
    QStringList leveysLista = kp()->settings()->value("KirjausWgRuudukko").toStringList();

    viewport()->installEventFilter(this);
}

void ViennitView::setTosite(Tosite *tosite)
{
    tosite_ = tosite;
    setModel( tosite->viennit());
}

void ViennitView::seuraavaSarake()
{
    const QModelIndex index = currentIndex();
    if( index.row() == model()->rowCount() -1 &&
        index.column() == model()->columnCount() -1)
    {
        // Lisätään uusi rivi
        TositeViennit *vientiModel = tosite_->viennit();
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
        if( (tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::VASTATTAVAA)) && qAbs(index.sibling(index.row(), TositeViennit::DEBET).data(Qt::EditRole).toDouble()) < 1e-3)
            setCurrentIndex( model()->index( index.row(), TositeViennit::KREDIT ) );
        else
            setCurrentIndex( model()->index( index.row(), TositeViennit::DEBET ) );
    }
    else
    {
        // Jos ALV-sarake on piilotettu, niin sen yli hypätään
        bool kohdennukset = kp()->kohdennukset()->kohdennuksia();

        if( isColumnHidden( index.column() + 1) )
            setCurrentIndex( model()->index( index.row(), index.column() + 2 ) );
        else if( index.column() == TositeViennit::DEBET && index.data(Qt::EditRole).toDouble() > 1e-5)
            setCurrentIndex( model()->index( index.row(), index.column() + ( kohdennukset ? 2 : 3) ));
        else if( index.column() == TositeViennit::KREDIT && !kohdennukset)
            setCurrentIndex( model()->index( index.row(), index.column() + 2 ) );
        else
            setCurrentIndex( model()->index( index.row(), index.column() + 1 ) );
    }


}

bool ViennitView::eventFilter(QObject *watched, QEvent *event)
{
    if( watched == viewport() && tosite_ && tosite_->viennit()->muokattavissa()) {
        if( event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if( mouseEvent->button() == Qt::RightButton)
            {
                QModelIndex index = indexAt( mouseEvent->pos() );
                if( index.column() == TositeViennit::KOHDENNUS && index.data(TositeViennit::PvmRooli).toDate().isValid() )  {

                    tosite_->viennit()->setData(index, KohdennusProxyModel::tagiValikko( index.data(TositeViennit::PvmRooli).toDate(),
                                                                                          index.data(TositeViennit::TagiIdListaRooli).toList(),
                                                                                          mouseEvent->globalPos()) ,
                                                   TositeViennit::TagiIdListaRooli);
                    return false;
                }

            }                            
        }
    }

    return QTableView::eventFilter(watched, event);
}

void ViennitView::keyPressEvent(QKeyEvent *event)
{
    if( event->modifiers() == Qt::KeyboardModifier::NoModifier &&
        event->key() == Qt::Key_Tab)
    {
        seuraavaSarake();
    } else if( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return ) {
        const QModelIndex index = currentIndex();
        seuraavaSarake();
        if( index.row() == model()->rowCount() -1 ) {            
            tosite_->viennit()->lisaaVienti(model()->rowCount());
        }

        setCurrentIndex( model()->index(index.row()+1, TositeViennit::PVM) );
    } else if( event->key() == Qt::Key_0 && currentIndex().column() == TositeViennit::ALV) {
        model()->setData(currentIndex(), AlvKoodi::EIALV, TositeViennit::AlvKoodiRooli);
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
    else if( hint == QAbstractItemDelegate::EditPreviousItem) {
        if( currentIndex().column() > 0)
            setCurrentIndex( model()->index( currentIndex().row(), currentIndex().column()-1 ) );
        else if( currentIndex().row() > 0)
            setCurrentIndex(model()->index( currentIndex().row()-1, TositeViennit::SELITE));
    }
}
