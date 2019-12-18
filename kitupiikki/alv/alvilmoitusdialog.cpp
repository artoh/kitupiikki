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

#include <QSqlQuery>
#include <QDebug>
#include <QPrinter>
#include <QPainter>
#include <QSqlError>

#include <QTextDocument>
#include <QMessageBox>
#include "alvilmoitusdialog.h"
#include "ui_alvilmoitusdialog.h"

#include "db/kirjanpito.h"


#include "alvlaskelma.h"


AlvIlmoitusDialog::AlvIlmoitusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlvIlmoitusDialog)
{
    ui->setupUi(this);
}

AlvIlmoitusDialog::~AlvIlmoitusDialog()
{
    delete ui;
}

QDate AlvIlmoitusDialog::teeAlvIlmoitus(QDate alkupvm, QDate loppupvm)
{
    AlvIlmoitusDialog *dlg = new AlvIlmoitusDialog();
    AlvLaskelma *laskelma = new AlvLaskelma(dlg);

    connect(laskelma, &AlvLaskelma::valmis, dlg, &AlvIlmoitusDialog::naytaLaskelma);
    laskelma->laske(alkupvm, loppupvm);

    return QDate();
    /*

    // Tarkistetaan, että tarvittavat tilit löytyy

    if( !kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).onkoValidi() ||
        !kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).onkoValidi() ||
            !kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).onkoValidi() )
    {
        QMessageBox::critical(nullptr, tr("Kitupiikin virhe"),
                              tr("Alv-tilitystä ei voi laatia, koska tilikartta on puutteellinen."));
        return QDate();
    }

    AlvIlmoitusDialog dlg;

    // Maksuperusteisen alv:n lopettaminen

    if( alkupvm == kp()->asetukset()->pvm("MaksuAlvLoppuu"))
    {
        if( !dlg.maksuperusteisenTilitys(kp()->asetukset()->pvm("MaksuAlvLoppuu"), alkupvm ) )
            return QDate();
    }

    // Erääntynyt (12kk) maksuperusteinen alv
    else if( kp()->onkoMaksuperusteinenAlv( loppupvm ) )
    {
        if( !dlg.maksuperusteisenTilitys( alkupvm.addYears(-1), alkupvm ) )
            return QDate();
    }


    if( dlg.alvIlmoitus(alkupvm, loppupvm))
        return loppupvm;
    else
        return QDate();
    */
}


void AlvIlmoitusDialog::otsikko(const QString &teksti)
{
    RaporttiRivi rivi;
    kirjoittaja->lisaaRivi();
    rivi.lisaa(teksti);
    rivi.lihavoi();
    kirjoittaja->lisaaRivi(rivi);
}

void AlvIlmoitusDialog::luku(const QString &nimike, qlonglong senttia, bool viiva)
{
    RaporttiRivi rivi;
    rivi.lisaa(nimike);
    rivi.lisaa( senttia ,true);
    if( viiva )
        rivi.viivaYlle(true);
    kirjoittaja->lisaaRivi(rivi);
}


void AlvIlmoitusDialog::naytaLaskelma(RaportinKirjoittaja rk)
{
    AlvLaskelma *laskelma = qobject_cast<AlvLaskelma*>( sender() );
    ui->ilmoitusBrowser->setHtml( rk.html() );
    ui->huojennusCheck->setVisible( laskelma->huojennus() && kp()->asetukset()->onko("AlvHuojennusTili") );
    if( exec() ) {
        if( ui->huojennusCheck->isChecked())
            laskelma->kirjaaHuojennus();
        laskelma->tallenna();
    }
}

