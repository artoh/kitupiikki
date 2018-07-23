/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "naytinikkuna.h"

#include "naytinview.h"

#include <QSettings>
#include <QAction>
#include <QToolButton>
#include <QToolBar>
#include <QMenu>

#include <QDialog>
#include "ui_csvvientivalinnat.h"


NaytinIkkuna::NaytinIkkuna(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    QSettings settings;
    resize(800,600);
    restoreGeometry( settings.value("NaytinIkkuna").toByteArray());

    view_ = new NaytinView(this);
    connect( view_, &NaytinView::sisaltoVaihtunut, this, &NaytinIkkuna::sisaltoMuuttui);

    setCentralWidget(view_);

    teeToolbar();

}

NaytinIkkuna::~NaytinIkkuna()
{
    QSettings settings;
    settings.setValue("NaytinIkkuna", saveGeometry());
}

void NaytinIkkuna::naytaRaportti(RaportinKirjoittaja raportti)
{
    NaytinIkkuna *ikkuna = new NaytinIkkuna;
    ikkuna->show();
    ikkuna->view()->nayta(raportti);
}

void NaytinIkkuna::sisaltoMuuttui(const QString& tyyppi)
{
    setWindowTitle( view()->naytinScene()->otsikko() );

    // Tyypin mukaan napit
}

void NaytinIkkuna::csvAsetukset()
{
    QSettings settings;
    QChar erotin = settings.value("CsvErotin", QChar(',')).toChar();
    QChar despilkku = settings.value("CsvDesimaali", QChar(',')).toChar();
    QString pvmmuoto = settings.value("CsvPaivays", "dd.MM.yyyy").toString();
    QString koodaus = settings.value("CsvKoodaus","utf8").toString();

    QDialog dlg;
    Ui::CsvVientiValintaDlg ui;
    ui.setupUi(&dlg);

    ui.eropilkku->setChecked( erotin == ',' );
    ui.eropuolipiste->setChecked( erotin == ';' );
    ui.erosarakain->setChecked( erotin == '\t' );

    ui.utf8->setChecked( koodaus == "utf8");
    ui.latin1->setChecked( koodaus == "latin1");

    ui.desipilkku->setChecked( despilkku == ',');
    ui.desipiste->setChecked( despilkku == '.');

    ui.suomipaiva->setChecked( pvmmuoto == "dd.MM.yyyy");
    ui.isopaiva->setChecked( pvmmuoto == "yyyy-MM-dd");
    ui.usapaiva->setChecked( pvmmuoto == "MM/dd/yyyy");

    if( dlg.exec())
    {
        if( ui.eropilkku->isChecked())
            settings.setValue("CsvErotin", QChar(','));
        else if( ui.eropuolipiste->isChecked())
            settings.setValue("CsvErotin", QChar(';'));
        else if( ui.erosarakain->isChecked())
            settings.setValue("CsvErotin", QChar('\t'));

        if( ui.utf8->isChecked())
            settings.setValue("CsvKoodaus", "utf8");
        else if( ui.latin1->isChecked())
            settings.setValue("CsvKoodaus", "latin1");

        if( ui.desipilkku->isChecked())
            settings.setValue("CsvDesimaali", QChar(','));
        else if( ui.desipiste->isChecked())
            settings.setValue("CsvDesimaali", QChar('.'));

        if( ui.suomipaiva->isChecked())
            settings.setValue("CsvPaivays","dd.MM.yyyy");
        else if( ui.isopaiva->isChecked())
            settings.setValue("CsvPaivays", "yyyy-MM-dd");
        else if( ui.usapaiva->isChecked())
            settings.setValue("CsvPaivays","MM/dd/yyyy");
    }
}

void NaytinIkkuna::teeToolbar()
{
    QToolBar *tb = addToolBar(tr("Ikkuna"));

    tb->addAction(QIcon(":/pic/peru.png"), tr("Sulje"), this, SLOT(close()));


    tb->addSeparator();

    htmlNappi_ = new QToolButton;
    htmlNappi_->setIcon( QIcon(":/pic/web.png"));
    htmlNappi_->setText( tr("HTML"));

    tb->addWidget(htmlNappi_);

    csvNappi_ = new QToolButton();
    csvNappi_->setIcon( QIcon(":/pic/csv.png"));
    csvNappi_->setText(tr("CSV"));
    QMenu *csvValikko = new QMenu();

    QAction *csvLeikepoydelleAktio = new QAction( QIcon(":/pic/csv.png"), tr("Leikepöydälle") );
    connect( csvLeikepoydelleAktio, &QAction::triggered, [this] { this->view()->naytinScene()->csvLeikepoydalle(); });
    csvValikko->addAction(csvLeikepoydelleAktio);
    QAction *csvAsetukset = new QAction( QIcon(":/pic/ratas.png"), tr("CSV:n muoto"));
    connect( csvAsetukset, &QAction::triggered, this, &NaytinIkkuna::csvAsetukset);
    csvValikko->addAction(csvAsetukset);


    csvNappi_->setMenu(csvValikko);
    tb->addWidget(csvNappi_);
}

