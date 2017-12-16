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

#include <QMessageBox>

#include "tilinpaattaja.h"
#include "db/kirjanpito.h"

#include "ui_tilinpaattaja.h"
#include "ui_lukitsetilikausi.h"

#include "arkistosivu.h"
#include "poistaja.h"

TilinPaattaja::TilinPaattaja(Tilikausi kausi, QWidget *parent) :
    QDialog(parent),
    tilikausi(kausi),
    ui(new Ui::TilinPaattaja)
{
    ui->setupUi(this);
    paivitaDialogi();

    connect( ui->lukitseNappi, SIGNAL(clicked(bool)), this, SLOT(lukitse()));
    connect( ui->poistoNappi, SIGNAL(clicked(bool)), this, SLOT(teePoistot()));
}

TilinPaattaja::~TilinPaattaja()
{
    delete ui;
}

void TilinPaattaja::paivitaDialogi()
{
    QString varoitukset;

    bool lukittu = kp()->tilitpaatetty() >= tilikausi.paattyy();

    ui->otsikkoLabel->setText(tr("Tilinpäätös tilikaudelle %1 - %2")
                              .arg(tilikausi.alkaa().toString(Qt::SystemLocaleShortDate))
                              .arg(tilikausi.paattyy().toString(Qt::SystemLocaleShortDate)));

    ui->valmisteluRyhma->setEnabled( !lukittu);
    ui->lukitseNappi->setVisible(!lukittu);
    ui->lukitseTehty->setVisible(lukittu);
    ui->tilinpaatosNappi->setEnabled(lukittu);
    ui->tulostaNappi->setEnabled( tilikausi.tilinpaatoksenTila() == Tilikausi::KESKEN);
    ui->vahvistaNappi->setEnabled( tilikausi.tilinpaatoksenTila() == Tilikausi::KESKEN);

    ui->poistoNappi->setEnabled( Poistaja::onkoPoistoja(tilikausi));


    if( kp()->paivamaara() < tilikausi.paattyy() )
    {
        varoitukset.append(tr("<p><b>Tilikausi on vielä kesken</b><br>"
                              "Jatka tilinpäätösen laatimista vain, mikäli olet täysin "
                              "varma siitä, että kaikki tilikaudelle kuuluvat kirjaukset on jo tehty."));
    }

    if( kp()->asetukset()->onko("AlvVelvollinen") && kp()->asetukset()->pvm("AlvIlmoitus") < tilikausi.paattyy())
    {
        // Alv-ilmoitusta ei ole tehty koko tilikaudelle!
        varoitukset.append( tr("<p><b>Arvonlisäveroilmoitus on tehty vasta %1 asti.</b></p>")
                            .arg( kp()->asetukset()->pvm("AlvIlmoitus").toString(Qt::SystemLocaleShortDate)));
    }

    ui->varoKuvake->setVisible( !varoitukset.isEmpty() );
    ui->varoLabel->setText(varoitukset);
}

void TilinPaattaja::lukitse()
{
    if( ui->varoLabel->text().length())
    {
        // Lisävaroitus jos huomioitavaa ...

        QString varoitus = tr("<p><b>Haluatko todella lukita tilikauden alla olevista varoituksista "
                              "huolimatta: </b></p> %1").arg(ui->varoLabel->text());
        if( QMessageBox::critical(this, tr("Tilinpäätöksen laatiminen"),
                                  varoitus, QMessageBox::Yes | QMessageBox::Cancel) != QMessageBox::Yes )
            return;
    }
    // Sitten kirjanpidon lukitseminen ja siihen liittyvä varoitus
    QDialog dlg;
    Ui::LukitseTilikausi ui;
    ui.setupUi( &dlg );
    if( dlg.exec() != QDialog::Accepted)
        return;

    // Lukitaan tilikausi!
    kp()->asetukset()->aseta("TilitPaatetty", tilikausi.paattyy());
    // Laaditaan arkisto
    emit lukittu(tilikausi);

    paivitaDialogi();
}

void TilinPaattaja::teePoistot()
{
    Poistaja::teeSumuPoistot(tilikausi);
}
