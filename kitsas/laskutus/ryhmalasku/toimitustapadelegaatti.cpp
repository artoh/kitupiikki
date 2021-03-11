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
#include "toimitustapadelegaatti.h"

#include <QComboBox>
#include "../vanhalaskudialogi.h"
#include "laskutettavatmodel.h"

ToimitustapaDelegaatti::ToimitustapaDelegaatti(QObject *parent) :
    QItemDelegate(parent)
{

}

QWidget *ToimitustapaDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/, const QModelIndex &/*index*/) const
{
    QComboBox *cbox = new QComboBox(parent);
    return cbox;
}

void ToimitustapaDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    alustaCombobox(cbox, index.data(LaskutettavatModel::LahetysTavatRooli).toList());
    cbox->setCurrentIndex( cbox->findData(index.data(Qt::EditRole)) );
}

void ToimitustapaDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    model->setData(index, cbox->currentData());
}



QString ToimitustapaDelegaatti::toimitustapa(int koodi)
{
    switch (koodi) {
    case Lasku::TULOSTETTAVA:
        return tr("Tulosta");
    case Lasku::SAHKOPOSTI:
        return tr("Sähköposti");
    case Lasku::POSTITUS:
        return tr("Postitus");
    case Lasku::VERKKOLASKU:
        return tr("Verkkolasku");
    case Lasku::PDF:
        return tr("PDF");
    case Lasku::EITULOSTETA:
        return tr("Ei tulosteta");
    case Lasku::TUOTULASKU:
        return tr("Tuotu lasku");

    default:
        return QString();
    }

}

QIcon ToimitustapaDelegaatti::icon(int koodi)
{
    switch (koodi) {
    case Lasku::TULOSTETTAVA:
        return QIcon(":/pic/tulosta.png");
    case Lasku::SAHKOPOSTI:
        return QIcon(":/pic/email.png");
    case Lasku::POSTITUS:
        return QIcon(":/pic/mail.png");
    case Lasku::VERKKOLASKU:
        return QIcon(":/pic/verkkolasku.png");
    case Lasku::PDF:
        return QIcon(":/pic/pdf.png");
    case Lasku::EITULOSTETA:
        return QIcon(":/pic/tyhja.png");
    case Lasku::TUOTULASKU:
        return QIcon(":/pic/tuotiedosto.png");
    default:
        return QIcon(":/tyhja.png");
    }
}

void ToimitustapaDelegaatti::alustaCombobox(QComboBox *combo, const QVariantList &tavat)
{
    for(auto item : tavat) {
        int koodi = item.toInt();
        combo->addItem( icon(koodi), toimitustapa(koodi), koodi);
    }
}
