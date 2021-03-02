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
#include "kielilistwidget.h"

#include "monikielinen.h"
#include "kielet.h"

KieliListWidget::KieliListWidget(QWidget *parent) :
    QListWidget(parent)
{

}

void KieliListWidget::lataa(const AbstraktiMonikielinen &monikielinen)
{
    clear();
    for( const Kieli& kieli : Kielet::instanssi()->kielet()) {
        QString kaannos = monikielinen.kaannos(kieli.lyhenne());
        QListWidgetItem* item = new QListWidgetItem( QIcon(kieli.lippu()), kaannos , this  );
        item->setData(Qt::UserRole, kieli.lyhenne());
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }
}

Monikielinen KieliListWidget::tekstit() const
{
    Monikielinen data;
    for(int i=0; i < count(); i++) {
        QListWidgetItem *wi = item(i);
        if( !wi->text().isEmpty() )
            data.aseta(wi->text(), wi->data(Qt::UserRole).toString());
    }
    return data;
}
