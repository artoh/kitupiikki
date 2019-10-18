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
#include <QDesktopServices>
#include <QUrl>
#include <QFile>

#include "tilinpaattaja.h"
#include "db/kirjanpito.h"
#include "tilinpaatoseditori/tilinpaatoseditori.h"

#include "ui_tilinpaattaja.h"
#include "ui_lukitsetilikausi.h"

#include "arkistosivu.h"
#include "poistaja.h"
#include "jaksottaja.h"

#include "arkisto/arkistosivu.h"

#include "naytin/naytinikkuna.h"

TilinPaattaja::TilinPaattaja(Tilikausi kausi,ArkistoSivu *arkisto , QWidget *parent) :
    QDialog(parent),
    tilikausi(kausi),
    arkistosivu(arkisto),
    ui(new Ui::TilinPaattaja)
{
    ui->setupUi(this);
    paivitaDialogi();

    connect( ui->lukitseNappi, SIGNAL(clicked(bool)), this, SLOT(lukitse()));
    connect( ui->poistoNappi, SIGNAL(clicked(bool)), this, SLOT(teePoistot()));
    connect( ui->jaksotusNappi, &QPushButton::clicked, this, &TilinPaattaja::teeJaksotukset);
    connect( ui->tilinpaatosNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect( ui->tulostaNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));
    connect( ui->vahvistaNappi, SIGNAL(clicked(bool)), this, SLOT(vahvista()));

    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("tilikaudet/tilinpaatos"); });
}

TilinPaattaja::~TilinPaattaja()
{
    delete ui;
}

void TilinPaattaja::paivitaDialogi()
{
    // Haetaan tilikausi uudelleen tietokannasta siltä varalta, että vaikkapa
    // poistotieto olisi päivittynyt ...

    tilikausi = kp()->tilikaudet()->tilikausiPaivalle( tilikausi.paattyy() );

    QString varoitukset;

    bool lukittu = kp()->tilitpaatetty() >= tilikausi.paattyy();

    ui->otsikkoLabel->setText(tr("Tilinpäätös tilikaudelle %1 - %2")
                              .arg(tilikausi.alkaa().toString("dd.MM.yyyy"))
                              .arg(tilikausi.paattyy().toString("dd.MM.yyyy")));

    ui->valmisteluRyhma->setEnabled( !lukittu);
    ui->lukitseNappi->setVisible(!lukittu);
    ui->lukitseTehty->setVisible(lukittu);
    ui->lukittuLabel->setVisible(lukittu);
    ui->tilinpaatosNappi->setEnabled(lukittu);

    bool tilinpaatosolemassa = QFile::exists( kp()->arkistopolku() + "/" + tilikausi.arkistoHakemistoNimi() + "/tilinpaatos.pdf"  );

    ui->tulostaNappi->setEnabled( tilinpaatosolemassa );
    ui->vahvistaNappi->setEnabled( tilinpaatosolemassa );



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
                            .arg( kp()->asetukset()->pvm("AlvIlmoitus").toString("dd.MM.yyyy")));
    }

    ui->varoKuvake->setVisible( !varoitukset.isEmpty() );
    ui->varoLabel->setText(varoitukset);

    KpKysely *kysely = kpk(QString("/tilikaudet/%1").arg(tilikausi.paattyy().toString(Qt::ISODate)));
    connect( kysely, &KpKysely::vastaus, this, &TilinPaattaja::dataSaapuu);
    kysely->kysy();

    kp()->tilikaudet()->paivita();
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
    QDialog dlg(this);
    Ui::LukitseTilikausi ui;
    ui.setupUi( &dlg );
    if( dlg.exec() != QDialog::Accepted)
        return;

    // Lukitaan tilikausi!
    kp()->asetukset()->aseta("TilitPaatetty", tilikausi.paattyy());
    // Laaditaan arkisto
    // arkistosivu->teeArkisto(tilikausi);

    paivitaDialogi();
}

void TilinPaattaja::teePoistot()
{
    Poistaja *poistaja = new Poistaja(this);
    connect( poistaja, &Poistaja::poistettu, this, &TilinPaattaja::paivitaDialogi);
    poistaja->teepoistot(tilikausi, data_.value("poistot").toList());
}

void TilinPaattaja::teeJaksotukset()
{
    Jaksottaja *jaksottaja = new Jaksottaja(this);
    connect( jaksottaja, &Jaksottaja::jaksotettu, this, &TilinPaattaja::paivitaDialogi);
    jaksottaja->teeJaksotukset(tilikausi, data_.value("jaksotukset").toList());
}

void TilinPaattaja::muokkaa()
{
    TilinpaatosEditori *editori = new TilinpaatosEditori(tilikausi, parentWidget() );
    editori->show();
    editori->move( parentWidget()->mapToGlobal( QPoint(25,25) ) );
    editori->resize( parentWidget()->size() );

    connect( editori, SIGNAL(tallennettu()), this, SLOT(paivitaDialogi()));
    connect( this, SIGNAL(vahvistettu()), editori, SLOT(close()));
}

void TilinPaattaja::esikatsele()
{
    // Avataan tilinpäätös
//    NaytinIkkuna::nayta( kp()->liitteet()->liite( tilikausi.alkaa().toString(Qt::ISODate) ) );
}

void TilinPaattaja::vahvista()
{
    if( QMessageBox::question(this, tr("Vahvista tilinpäätös"),
                              tr("Onko tilinpäätös vahvistettu lopulliseksi?\n"
                                 "Vahvistettua tilinpäätöstä ei voi enää muokata.")) != QMessageBox::Yes)
        return;

    tilikausi.set("Vahvistettu", kp()->paivamaara());
    tilikausi.tallenna();
    emit kp()->onni("Tilinpäätös merkitty valmiiksi");
    emit vahvistettu();
    close();
}

void TilinPaattaja::dataSaapuu(QVariant *data)
{
    data_ = data->toMap();

    bool poistotkirjattu = data_.value("poistot").toString() == "kirjattu";
    bool eipoistoja = data_.value("poistot").toList().isEmpty();

    ui->eiPoistettavaaLabel->setVisible(eipoistoja && !poistotkirjattu);
    ui->poistoTehty->setVisible( poistotkirjattu );
    ui->poistotKirjattuLabel->setVisible( poistotkirjattu );

    ui->poistoNappi->setVisible(!poistotkirjattu && !eipoistoja);

    bool jaksotuksetkirjattu = data_.value("jaksotukset").toString() == "kirjattu";
    bool eijaksotuksia = data_.value("jaksotukset").toList().isEmpty();

    ui->eiJaksotettavaaLabel->setVisible( !jaksotuksetkirjattu && eijaksotuksia);
    ui->jaksotTehty->setVisible( jaksotuksetkirjattu );
    ui->jaksotKirjattuLabel->setVisible( jaksotuksetkirjattu );
    ui->jaksotusNappi->setVisible(!jaksotuksetkirjattu && !eijaksotuksia);

}
