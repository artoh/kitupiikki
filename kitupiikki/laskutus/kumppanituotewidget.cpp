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
#include "kumppanituotewidget.h"
#include "ui_kumppanituotewidget.h"

#include "asiakkaatmodel.h"
#include "tuotemodel.h"
#include <QSortFilterProxyModel>

KumppaniTuoteWidget::KumppaniTuoteWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KumppaniTuoteWidget),
    proxy_(new QSortFilterProxyModel),
    asiakkaat_( new AsiakkaatModel(this)),
    tuotteet_( new TuoteModel(this))
{
    ui->setupUi(this);

    proxy_->setDynamicSortFilter(true);
    proxy_->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->view->setSortingEnabled(true);
    ui->view->setModel(proxy_);

    connect( ui->view->selectionModel(), &QItemSelectionModel::selectionChanged,
             this, &KumppaniTuoteWidget::ilmoitaValinta);
}

KumppaniTuoteWidget::~KumppaniTuoteWidget()
{
    delete ui;
}

void KumppaniTuoteWidget::nayta(int valilehti)
{
   if( valilehti == TUOTTEET)
       proxy_->setSourceModel( tuotteet_ );
   else
       proxy_->setSourceModel( asiakkaat_);

   if( valilehti == ASIAKKAAT )
       asiakkaat_->paivita(false);
   else if( valilehti == TOIMITTAJAT)
       asiakkaat_->paivita(true);
   else if( valilehti == TUOTTEET)
       tuotteet_->lataa();

   ui->view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void KumppaniTuoteWidget::suodata(const QString &suodatus)
{
    proxy_->setFilterFixedString( suodatus );
}

void KumppaniTuoteWidget::ilmoitaValinta()
{
    if( !ui->view->selectionModel()->selectedRows(0).isEmpty() )
        emit ui->view->selectionModel()->selectedRows(0).value(0).data().toString();
}
