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

#include "db/tilinvalintaline.h"

#include "tilidelegaatti.h"

#include "model/tositeviennit.h"

#include "db/tilinvalintadialogi.h"

TiliDelegaatti::TiliDelegaatti(QObject *parent) :
    QItemDelegate(parent)
{

}

QWidget *TiliDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    TilinvalintaLineDelegaatille *editor = new TilinvalintaLineDelegaatille(parent);
    editor->etsiKayttoon(etsiKaytossa_);
    if(naytaKaikki_) editor->naytaKaikki();
    return editor;
}

void TiliDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TilinvalintaLineDelegaatille *tilinvalinta = qobject_cast<TilinvalintaLineDelegaatille*>(editor);
    tilinvalinta->valitseTiliNumerolla( index.data(TositeViennit::TiliNumeroRooli).toInt() );

}

void TiliDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TilinvalintaLineDelegaatille *tilieditor = qobject_cast<TilinvalintaLineDelegaatille*>(editor);

    QString alku = tilieditor->tilinimiAlkaa();

    if( !alku.isEmpty() && etsiKaytossa_) {

        TilinValintaDialogi* dlg = new TilinValintaDialogi();
        if( alku.startsWith("*")) {
            dlg->valitse(alku.mid(1).toInt());
        } else {
            dlg->suodata(alku);
        }

        int rivi = index.row();
        int sarake = index.column();
        connect(dlg, &TilinValintaDialogi::tiliValittu, this,
                [model, rivi, sarake] (int tilinumero) { model->setData( model->index(rivi, sarake), tilinumero);} );

        dlg->show();

        // model->setData(index, tilieditor->tilinimiAlkaa() );
    }
    else
        model->setData(index, tilieditor->valittuTilinumero());

}

void TiliDelegaatti::etsiKayttoon(bool onko)
{
    etsiKaytossa_ = onko;
}

void TiliDelegaatti::naytaKaikki()
{
    naytaKaikki_ = true;
}





