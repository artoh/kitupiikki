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
#include "tilioteview.h"

#include "kirjaus/pvmdelegaatti.h"
#include "kirjaus/tilidelegaatti.h"
#include "kirjaus/eurodelegaatti.h"
#include "kirjaus/kohdennusdelegaatti.h"
#include "tiliotealvdelegaatti.h"

#include "tiliotemodel.h"

#include "rekisteri/kumppanivalintadelegaatti.h"

#include "tilioterivi.h"
#include "db/kirjanpito.h"
#include "db/asetusmodel.h"

#include <QHeaderView>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QKeyEvent>

TilioteView::TilioteView(QWidget *parent) :
    QTableView(parent)
{
    setItemDelegateForColumn( TilioteRivi::PVM, new PvmDelegaatti(this) );
    setItemDelegateForColumn( TilioteRivi::TILI, new TiliDelegaatti(this) );
    setItemDelegateForColumn( TilioteRivi::EURO, new EuroDelegaatti(this) );
    setItemDelegateForColumn( TilioteRivi::ALV, new TilioteAlvDelegaatti(this));
    setItemDelegateForColumn( TilioteRivi::KOHDENNUS, new KohdennusDelegaatti(this) );

    setItemDelegateForColumn(TilioteRivi::SAAJAMAKSAJA,
                             new KumppaniValintaDelegaatti(this));

    sortByColumn(TilioteRivi::PVM, Qt::AscendingOrder);

    setFocusPolicy(Qt::StrongFocus);

    QTimer::singleShot(15, this, [this] {this->horizontalHeader()->setSectionResizeMode( TilioteRivi::SELITE, QHeaderView::Stretch );});
    QTimer::singleShot(10, this, &TilioteView::palautaRuudukonGeometria);

}

TilioteView::~TilioteView()
{
    const bool naytaAlv = kp()->asetukset()->onko(AsetusModel::AlvVelvollinen);
    const QString ruudukkoNimi = naytaAlv ? "TilioteruudukkoAlvilla" : "Tilioteruudukko";
    kp()->settings()->setValue(ruudukkoNimi, horizontalHeader()->saveState());
}

void TilioteView::setModel(QAbstractItemModel *model)
{
//    connect( model, &QAbstractTableModel::modelAboutToBeReset, this, &TilioteView::ennenResetia);
//    connect( model, &QAbstractTableModel::modelReset, this, &TilioteView::resetinJalkeen);

    QTableView::setModel(model);
}

void TilioteView::keyPressEvent(QKeyEvent *event)
{

    if( event->key() == Qt::Key_Insert) {        
        model()->insertRow(currentIndex().row()+1);
        setCurrentIndex(currentIndex().sibling(currentIndex().row()+1, TilioteRivi::PVM));
        return;
    } else if( ( event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Insert ||
        event->key() == Qt::Key_Tab) &&
            (event->modifiers() == Qt::NoModifier ||
             event->modifiers() == Qt::KeypadModifier )  ){

        if( currentIndex().column() == TilioteRivi::EURO &&
            currentIndex().row() == model()->rowCount() - 1) {
            model()->insertRow(currentIndex().row()+1);
            setCurrentIndex(currentIndex().sibling(currentIndex().row()+1, TilioteRivi::PVM));
            return;
        } else if( event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
            if(currentIndex().row() == model()->rowCount() - 1 && qAbs(currentIndex().sibling(currentIndex().row(), TilioteRivi::EURO).data(Qt::EditRole).toDouble()) > 1e-5) {
                model()->insertRow(currentIndex().row()+1);
            }
            int uusirivi = currentIndex().row() + 1;
            while(currentIndex().sibling(uusirivi,0).isValid() && (currentIndex().sibling(uusirivi,0).flags() & Qt::ItemIsEnabled) == 0) {
                uusirivi++;
            }
            setCurrentIndex(currentIndex().sibling(uusirivi, currentIndex().column()));
        }

    } else if( currentIndex().column() ==  TilioteRivi::ALV) {
        const int tiliNumero = currentIndex().data(TilioteRivi::TiliRooli).toInt();
        const Tili* tili = kp()->tilit()->tili(tiliNumero);
        if( !tili  || !tili->onko(TiliLaji::TULOS) ) {
            QTableView::keyPressEvent(event);
            return;
        }
        const int koodi = tili->onko(TiliLaji::TULO) ? AlvKoodi::MYYNNIT_NETTO : AlvKoodi::OSTOT_NETTO;

        if( event->key() == Qt::Key_0) {
            model()->setData(currentIndex(), 0);
            return;
        } else if( event->key() == Qt::Key_2) {
            const int nykyinen = currentIndex().data(Qt::EditRole).toInt() / 100;
            if( nykyinen == 2400)
                model()->setData(currentIndex(), koodi + 255000);
            else if(nykyinen == 2550)
                model()->setData(currentIndex(), koodi + 240000);
            else
                model()->setData(currentIndex(), koodi + yleinenAlv(currentIndex().data(TilioteKirjausRivi::PvmRooli).toDate()) * 100);
            return;
        } else if( event->key() == Qt::Key_1) {
            const int alv = currentIndex().data(Qt::EditRole).toInt();
            model()->setData(currentIndex(), alv / 100 == 1400 ? koodi + 100000 : koodi + 140000);
            return;
        } else if( event->key() == Qt::Key_T) {
            model()->setData(currentIndex(), tili->onko(TiliLaji::TULO) ? AlvKoodi::YHTEISOMYYNTI_TAVARAT : AlvKoodi::YHTEISOHANKINNAT_TAVARAT );
            return;
        } else if( event->key() == Qt::Key_P) {
            model()->setData(currentIndex(), tili->onko(TiliLaji::TULO) ? AlvKoodi::YHTEISOMYYNTI_PALVELUT : AlvKoodi::YHTEISOHANKINNAT_PALVELUT );
            return;
        } else if( event->key() == Qt::Key_R) {
            model()->setData(currentIndex(), tili->onko(TiliLaji::TULO) ? AlvKoodi::RAKENNUSPALVELU_MYYNTI : AlvKoodi::RAKENNUSPALVELU_OSTO );
            return;
        }
    }


    QTableView::keyPressEvent(event);
}

void TilioteView::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    QTableView::closeEditor(editor, QAbstractItemDelegate::NoHint);
    if( hint == QAbstractItemDelegate::EditNextItem) {
        if( currentIndex().column() == TilioteRivi::EURO) {
            if( currentIndex().row() == model()->rowCount() + 1) {
                model()->insertRow(currentIndex().row() + 1);
            }
            setCurrentIndex(currentIndex().sibling(currentIndex().row() + 1, TilioteRivi::PVM));
        } else {
            setCurrentIndex(currentIndex().sibling(currentIndex().row(), currentIndex().column() + 1));
        }

    } else if( hint == QAbstractItemDelegate::EditPreviousItem) {
        if( currentIndex().column() > 0)
            setCurrentIndex( model()->index( currentIndex().row(), currentIndex().column()-1 ) );
        else if( currentIndex().row() > 0)
            setCurrentIndex(model()->index( currentIndex().row()-1, TilioteRivi::EURO));
    }

}

void TilioteView::ennenResetia()
{
    lisaysIndeksi_ = selectionModel()->currentIndex().data(TilioteRivi::LisaysIndeksiRooli).toInt();
}

void TilioteView::resetinJalkeen()
{

    if( lisaysIndeksi_) {
        for(int i=0; i < model()->rowCount(); i++) {
            const QModelIndex& index = model()->index(i, TilioteRivi::SELITE);
            if(index.data(TilioteRivi::LisaysIndeksiRooli).toInt() == lisaysIndeksi_) {
                setCurrentIndex(index);

                break;
            }
        }
    }
}

void TilioteView::palautaRuudukonGeometria()
{
    const bool naytaAlv = kp()->asetukset()->onko(AsetusModel::AlvVelvollinen);
    const QString ruudukkoNimi = naytaAlv ? "TilioteruudukkoAlvilla" : "Tilioteruudukko";
    this->horizontalHeader()->restoreState(kp()->settings()->value(ruudukkoNimi).toByteArray());
    setColumnHidden( TilioteRivi::ALV, !naytaAlv);
}



