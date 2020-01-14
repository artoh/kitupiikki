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
#include "kielidelegaatti.h"

#include <QComboBox>

KieliDelegaatti::KieliDelegaatti(QObject *parent) :
    QItemDelegate(parent)
{

}

QWidget *KieliDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/, const QModelIndex &/*index*/) const
{
    QComboBox *cbox = new QComboBox(parent);
    alustaKieliCombo(cbox);
    return cbox;
}

void KieliDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    QString kielikoodi = index.data(Qt::EditRole).toString();
    cbox->setCurrentIndex(cbox->findData(kielikoodi));
}

void KieliDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cbox = qobject_cast<QComboBox*>(editor);
    QString kielikoodi = cbox->currentData().toString();
    model->setData(index, kielikoodi, Qt::EditRole);
}

void KieliDelegaatti::alustaKieliCombo(QComboBox *combo)
{
    combo->addItem(QIcon(":/liput/fi.png"),tr("suomi"),"FI");
    combo->addItem(QIcon(":/liput/sv.png"),tr("ruotsi"),"SV");
    combo->addItem(QIcon(":/liput/en.png"),tr("englanti"),"EN");
}

QString KieliDelegaatti::kieliKoodilla(const QString &kielikoodi)
{
    if( kielikoodi == "SV")
        return tr("ruotsi");
    else if( kielikoodi == "EN")
        return tr("englanti");
    else
        return tr("suomi");
}
