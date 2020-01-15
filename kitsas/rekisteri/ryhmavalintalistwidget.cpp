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
#include "ryhmavalintalistwidget.h"
#include "ryhmatmodel.h"
#include "db/kirjanpito.h"

RyhmaValintaListWidget::RyhmaValintaListWidget(QWidget *parent) :
    QListWidget(parent)
{
    lataaRyhmat();
}

QVariantList RyhmaValintaListWidget::valitutRyhmat() const
{
    QVariantList lista;
    for(int i=0; i<count(); i++) {
        QListWidgetItem* ci = item(i);
        if(ci->checkState() == Qt::Checked) {
            lista.append(ci->data(Qt::UserRole));
        }
    }
    return lista;
}

void RyhmaValintaListWidget::valitseRyhmat(const QVariantList &lista)
{
    for(int i=0; i < count(); i++) {
        QListWidgetItem *ci = item(i);
        ci->setCheckState( lista.contains( ci->data(Qt::UserRole) ) ?
                               Qt::Checked :
                               Qt::Unchecked);
    }
}

void RyhmaValintaListWidget::lataaRyhmat()
{
    clear();
    for(int i=1; i<kp()->ryhmat()->rowCount();i++) {
        QModelIndex index = kp()->ryhmat()->index(i,0);
        int koodi = index.data(RyhmatModel::IdRooli).toInt();
        QString nimi = index.data(Qt::DisplayRole).toString();
        QListWidgetItem* item = new QListWidgetItem(nimi, this);
        item->setData(Qt::UserRole, koodi);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    }
}
