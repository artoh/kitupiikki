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
#include "rekisteri/kumppanivalintadelegaatti.h"

#include "muumuokkausdlg.h"

#include <QHeaderView>
#include <QSettings>
#include <QKeyEvent>
#include <QTimer>

ViennitView::ViennitView(QWidget *parent)
    : QTableView (parent)
{
    setItemDelegateForColumn( TositeViennit::PVM, new PvmDelegaatti(this ) );
    setItemDelegateForColumn( TositeViennit::TILI, new TiliDelegaatti(this ) );
    setItemDelegateForColumn( TositeViennit::DEBET, new EuroDelegaatti(this));
    setItemDelegateForColumn( TositeViennit::KREDIT, new EuroDelegaatti(this));
    setItemDelegateForColumn( TositeViennit::KOHDENNUS, new KohdennusDelegaatti(this));
    setItemDelegateForColumn( TositeViennit::KUMPPANI, new KumppaniValintaDelegaatti(this));

    setEditTriggers( QTableView::AllEditTriggers );

    horizontalHeader()->setStretchLastSection(true);

    setFocusPolicy(Qt::StrongFocus);

    connect( this, &QTableView::doubleClicked, this, &ViennitView::muokkaa);

    viewport()->installEventFilter(this);
    QTimer::singleShot(10, this, [this] {this->horizontalHeader()->restoreState(kp()->settings()->value("ViennitRuudukko").toByteArray());});
    horizontalHeader()->setSectionsMovable(true);
}

ViennitView::~ViennitView()
{
    kp()->settings()->setValue("ViennitRuudukko", horizontalHeader()->saveState());
}

void ViennitView::setTosite(Tosite *tosite)
{
    tosite_ = tosite;
    setModel( tosite->viennit());
}

void ViennitView::seuraavaSarake()
{
    const QModelIndex index = currentIndex();
    TositeViennit* vientiModel = qobject_cast<TositeViennit*>(model());

    if( index.row() == model()->rowCount() -1 &&
        index.column() == model()->columnCount() -1 &&
        vientiModel &&
        vientiModel->muokattavissa())
    {
        // Lisätään uusi rivi
        TositeViennit *vientiModel = tosite_->viennit();
        vientiModel->lisaaVienti( model()->rowCount() );
        setCurrentIndex( vientiModel->index( model()->rowCount() - 1, TositeViennit::TILI ) );
    } else if( index.column() == model()->columnCount() - 1 )
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


    if( currentIndex().data(TositeViennit::TyyppiRooli).toInt() == TositeVienti::ALVKIRJAUS) {
        setCurrentIndex( currentIndex().sibling( currentIndex().row(), TositeViennit::SELITE  ) );
        seuraavaSarake();
    }

}

bool ViennitView::eventFilter(QObject *watched, QEvent *event)
{
    if( watched == viewport() && tosite_ && tosite_->viennit()->muokattavissa()) {
        if( event->type() == QEvent::MouseButtonPress)
        {            
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            QModelIndex index = indexAt( mouseEvent->pos() );
            if( mouseEvent->button() == Qt::RightButton)
            {
                #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                    QPoint globalPosition = mouseEvent->globalPosition().toPoint();
                #else
                    QPoint globalPosition = mouseEvent->globalPos();
                #endif

                if( index.column() == TositeViennit::KOHDENNUS && index.data(TositeViennit::PvmRooli).toDate().isValid() )  {

                    tosite_->viennit()->setData(index, KohdennusProxyModel::tagiValikko( index.data(TositeViennit::PvmRooli).toDate(),
                                                                                          index.data(TositeViennit::TagiIdListaRooli).toList(),
                                                                                          globalPosition),
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
    } else if( event->key() == Qt::Key_Backtab) {
        if( currentIndex().column() == 0) {
            if( currentIndex().row() > 0) {
                setCurrentIndex( currentIndex().sibling(currentIndex().row()-1, TositeViennit::SELITE) );
            }
        } else {
            if( isColumnHidden(currentIndex().column()-1))
                setCurrentIndex( currentIndex().sibling( currentIndex().row(), currentIndex().column() - 2 ) );
            else
                setCurrentIndex( currentIndex().sibling( currentIndex().row(), currentIndex().column() - 1 ) );
        }
    } else if( (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && currentIndex().flags() & Qt::ItemIsEditable ) {
        QModelIndex indeksi = currentIndex();        

        if(indeksi.data(TositeViennit::TiliNumeroRooli).toInt() == 0
                && indeksi.data(TositeViennit::DebetRooli).toDouble() < 1e-5
                && indeksi.data(TositeViennit::KreditRooli).toDouble() < 1e-5 ) {
            muokkaa(currentIndex());
        } else {
            while( currentIndex().data(TositeViennit::TyyppiRooli) == TositeVienti::ALVKIRJAUS && currentIndex().row() < model()->rowCount() - 1) {
                setCurrentIndex( currentIndex().sibling(currentIndex().row()+1, TositeViennit::TILI) );
            }
            if( currentIndex().row() == model()->rowCount() -1 ) {
                tosite_->viennit()->lisaaVienti(model()->rowCount());
            }
            if( currentIndex().row() < model()->rowCount() - 1) {
                setCurrentIndex( model()->index(indeksi.row()+1, TositeViennit::TILI) );
            }

        }
    } else if( currentIndex().column() != TositeViennit::SELITE && event->key() == Qt::Key_Space ) {
        muokkaa( currentIndex());
    } else if( currentIndex().column() == TositeViennit::ALV) {
        if(event->key() == Qt::Key_0) {
            model()->setData(currentIndex(), AlvKoodi::EIALV, TositeViennit::AlvKoodiRooli);
        } else if( event->key() == Qt::Key_Space) {
            muokkaa(currentIndex());
        } else if( event->key() == Qt::Key_2 || event->key() == Qt::Key_1) {
            int tilinumero = currentIndex().data(TositeViennit::TiliNumeroRooli).toInt();
            Tili* tili = kp()->tilit()->tili(tilinumero);
            double prosentti =
                    event->key() == Qt::Key_2 ? 24.0 :
                    currentIndex().data(TositeViennit::AlvProsenttiRooli).toInt() == 14 ? 10 : 14;
            model()->setData(currentIndex(), prosentti, TositeViennit::AlvProsenttiRooli);

            if(tili && kp()->alvTyypit()->nollaTyyppi(currentIndex().data(TositeViennit::AlvKoodiRooli).toInt())) {
                if(tili->onko(TiliLaji::TULO))
                    model()->setData(currentIndex(), AlvKoodi::MYYNNIT_NETTO, TositeViennit::AlvKoodiRooli);
                else if(tili->onko(TiliLaji::MENO))
                    model()->setData(currentIndex(), AlvKoodi::OSTOT_NETTO, TositeViennit::AlvKoodiRooli);

            }
        } else if( event->key() == Qt::Key_Asterisk) {
            double prs= (100 + currentIndex().data(TositeViennit::AlvProsenttiRooli).toDouble()) / 100.0;
            double debet = currentIndex().data(TositeViennit::DebetRooli).toDouble();
            double kredit = currentIndex().data(TositeViennit::KreditRooli).toDouble();
            if( prs > 1) {
                model()->setData(currentIndex().sibling(currentIndex().row(), TositeViennit::DEBET), debet / prs);
                model()->setData(currentIndex().sibling(currentIndex().row(), TositeViennit::KREDIT), kredit / prs);
            }
        }
    } else
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

void ViennitView::muokkaa(const QModelIndex &index)
{
    if(index.isValid() && tosite_->viennit()->muokattavissa()) {
        MuuMuokkausDlg dlg(this);
        TositeVienti vienti = tosite_->viennit()->vienti(index.row());
        dlg.muokkaa(vienti);
        if(dlg.exec() == QDialog::Accepted) {
            tosite_->viennit()->asetaVienti(index.row(), dlg.vienti());
        }
    }
}
