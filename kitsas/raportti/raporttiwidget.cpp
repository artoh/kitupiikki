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

#include <QDate>
#include <QFont>
#include <QPen>

#include <QFile>
#include <QUrl>
#include <QPrintDialog>
#include <QPageSetupDialog>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

#include <QPdfWriter>

#include <QCheckBox>
#include <QPushButton>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <QMimeData>
#include <QApplication>
#include <QClipboard>

#include <QPrinterInfo>

#include "raporttiwidget.h"
#include "db/kirjanpito.h"

#include <QSettings>
#include <QLabel>
#include <QApplication>

#include "naytin/naytinikkuna.h"


RaporttiWidget::RaporttiWidget(QWidget *parent) : QWidget(parent)
{
        raporttiWidget = new QWidget();

        odotaLabel = new QLabel(tr("Raporttia luodaan..."));
        odotaLabel->setStyleSheet("background-color: yellow;");
        odotaLabel->hide();

        QPushButton *esikatseluBtn = new QPushButton(QIcon(":/pic/print.png"), tr("Esikatsele"));
        connect( esikatseluBtn, &QPushButton::clicked, this, &RaporttiWidget::esikatselu);

        QHBoxLayout *nappiLeiska = new QHBoxLayout;
        nappiLeiska->addStretch();
        nappiLeiska->addWidget(odotaLabel);
        nappiLeiska->addStretch();
        nappiLeiska->addWidget(esikatseluBtn);

        QVBoxLayout *paaLeiska = new QVBoxLayout;
        paaLeiska->addWidget(raporttiWidget);
        paaLeiska->addStretch();
        paaLeiska->addLayout(nappiLeiska);

        setLayout(paaLeiska);
}


void RaporttiWidget::nayta(RaportinKirjoittaja rk)
{
    if( rk.riveja())
        NaytinIkkuna::naytaRaportti( rk );
    else
        QMessageBox::information(this, tr("Ei raportoitavaa"),
                                 tr("Tekemilläsi valinnoilla muodostuu tyhjä raportti"));
    odotaLabel->hide();
}

void RaporttiWidget::esikatselu()
{
    odotaLabel->show();
    qApp->processEvents();
    esikatsele();
}
