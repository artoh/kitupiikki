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
#include <QSettings>

#include "tilinpaattaja.h"
#include "db/kirjanpito.h"
#include "tilinpaatoseditori/tilinpaatoseditori.h"
#include "arkistoija/aineistodialog.h"

#include "ui_tilinpaattaja.h"
#include "ui_lukitsetilikausi.h"

#include "arkistosivu.h"
#include "poistaja.h"
#include "jaksottaja.h"
#include "tuloverodialog.h"
#include "yksityistilienpaattaja.h"

#include "arkisto/arkistosivu.h"

#include "naytin/naytinikkuna.h"
#include "alv/alvilmoitustenmodel.h"


TilinPaattaja::TilinPaattaja(Tilikausi kausi,ArkistoSivu *arkisto , QWidget *parent) :
    QDialog(parent),
    tilikausi(kausi),
    arkistosivu(arkisto),
    ui(new Ui::TilinPaattaja)
{
    ui->setupUi(this);
    restoreGeometry( kp()->settings()->value("TilinpaatosDialogi").toByteArray());

    ui->tilioimattaFrame->hide();
    paivitaDialogi();

    connect( ui->lukitseNappi, SIGNAL(clicked(bool)), this, SLOT(lukitse()));
    connect( ui->poistoNappi, SIGNAL(clicked(bool)), this, SLOT(teePoistot()));
    connect( ui->jaksotusNappi, &QPushButton::clicked, this, &TilinPaattaja::teeJaksotukset);
    connect( ui->tuloveroNappi, &QPushButton::clicked, this, &TilinPaattaja::kirjaaTulovero);
    connect( ui->tilinpaatosNappi, SIGNAL(clicked(bool)), this, SLOT(muokkaa()));
    connect( ui->tulostaNappi, SIGNAL(clicked(bool)), this, SLOT(esikatsele()));
    connect( ui->mappiNappi, &QPushButton::clicked, this, &TilinPaattaja::mappi);
    connect( ui->vahvistaNappi, SIGNAL(clicked(bool)), this, SLOT(vahvista()));
    connect( ui->yksityistiliButton, &QPushButton::clicked, this, &TilinPaattaja::paataYksityisTilit);

    connect( ui->ohjeNappi, &QPushButton::clicked, [] { kp()->ohje("tilikaudet/tilinpaatos/"); });
}

TilinPaattaja::~TilinPaattaja()
{
    kp()->settings()->setValue("TilinpaatosDialogi", saveGeometry());
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
                              .arg(tilikausi.alkaa().toString("dd.MM.yyyy"),
                              tilikausi.paattyy().toString("dd.MM.yyyy")));

    ui->valmisteluRyhma->setEnabled( !lukittu);
    ui->lukitseNappi->setVisible(!lukittu);
    ui->lukitseTehty->setVisible(lukittu);
    ui->lukittuLabel->setVisible(lukittu);
    ui->tilinpaatosNappi->setEnabled(lukittu);

    if( kp()->paivamaara() < tilikausi.paattyy() )
    {
        varoitukset.append(tr("<p><b>Tilikausi on vielä kesken</b><br>"
                              "Jatka tilinpäätösen laatimista vain, mikäli olet täysin "
                              "varma siitä, että kaikki tilikaudelle kuuluvat kirjaukset on jo tehty."));
    }

    if( kp()->onkoAlvVelvollinen( tilikausi.paattyy() ) && !kp()->alvIlmoitukset()->onkoIlmoitettu( tilikausi.paattyy() ) )
    {
        // Alv-ilmoitusta ei ole tehty koko tilikaudelle!
        varoitukset.append( tr("<p><b>Arvonlisäilmoitusta ei ole annettu tilikauden loppuun saakka.</b></p>") );
    }

    ui->varoKuvake->setVisible( !varoitukset.isEmpty() );
    ui->varoLabel->setText(varoitukset);

    KpKysely *kysely = kpk(QString("/tilikaudet/%1").arg(tilikausi.paattyy().toString(Qt::ISODate)));
    connect( kysely, &KpKysely::vastaus, this, &TilinPaattaja::dataSaapuu);
    kysely->kysy();

    tilinpaatosOlemassa_ = false;
    KpKysely* tkysely = kpk(QString("/liitteet/0/TP_%1").arg(tilikausi.paattyy().toString(Qt::ISODate)), KpKysely::GET);
    connect( tkysely, &KpKysely::vastaus, this, [this, lukittu] () {
        this->ui->tulostaNappi->setEnabled(lukittu);
        this->ui->vahvistaNappi->setEnabled(lukittu);
        tilinpaatosOlemassa_ = true;
    });
    tkysely->kysy();

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
    if( QMessageBox::question(this, tr("Tilikauden lukitseminen"),
                              tr("Haluatko muodostaa lukitusta tilikaudesta sähköisen arkiston?"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) == QMessageBox::Yes)
        arkistosivu->teeArkisto(tilikausi);

    if( tilinpaatosOlemassa_ ) {
        QMessageBox::information(this, tr("Tilinpäätöksen päivittäminen"),
                                tr("Jos olet muokannut tilinpäätökseen vaikuttavia tietoja, muodosta tilinpäätösdokumentti uudelleen <b>Liitetiedot</b>-kohdasta."));
    }

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
    jaksottaja->teeJaksotukset(tilikausi, data_.value("jaksotukset").toList(), data_.value("verosaaminen").toDouble());
}

void TilinPaattaja::kirjaaTulovero()
{
    TuloveroDialog *dlg = new TuloveroDialog(this);
    connect(dlg, &TuloveroDialog::tallennettu, this, &TilinPaattaja::paivitaDialogi);
    dlg->alusta( data_.value("tulovero").toMap(), tilikausi);
    dlg->show();
}

void TilinPaattaja::muokkaa()
{
    TilinpaatosEditori *editori = new TilinpaatosEditori(tilikausi, this );
    connect( editori, &TilinpaatosEditori::tallennettu, this, &TilinPaattaja::paivitaDialogi );

    connect( this, SIGNAL(vahvistettu()), editori, SLOT(close()));

    editori->show();
    editori->move( parentWidget()->mapToGlobal( QPoint(25,25) ) );
    editori->resize( parentWidget()->size() );


}

void TilinPaattaja::esikatsele()
{
    // Avataan tilinpäätös
    NaytinIkkuna::naytaLiite(0, QString("TP_%1").arg(tilikausi.paattyy().toString(Qt::ISODate)) );
}

void TilinPaattaja::mappi()
{
    AineistoDialog* aineisto = new AineistoDialog();
    aineisto->aineisto(tilikausi.alkaa(),
                       kp()->asetukset()->asetus(AsetusModel::TilinpaatosKieli));
}

void TilinPaattaja::vahvista()
{
    if( QMessageBox::question(this, tr("Vahvista tilinpäätös"),
                              tr("Onko tilinpäätös vahvistettu lopulliseksi?\n"
                                 "Vahvistettua tilinpäätöstä ei voi enää muokata.")) != QMessageBox::Yes)
        return;

    tilikausi.set("vahvistettu", kp()->paivamaara());
    tilikausi.tallenna();
    emit kp()->onni("Tilinpäätös vahvistettu", Kirjanpito::Onnistui);
    emit vahvistettu();
    close();
}

void TilinPaattaja::paataYksityisTilit()
{
    YksityistilienPaattaja *dlg = new YksityistilienPaattaja(this);
    const QVariant map = data_.value("yksityistilit");
    dlg->alusta(tilikausi, map.toMap());
    connect( dlg, &YksityistilienPaattaja::tallennettu, this, &TilinPaattaja::paivitaDialogi);
    dlg->show();
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
    bool eijaksotuksia = data_.value("jaksotukset").toList().isEmpty() &&
            !data_.contains("verosaaminen");
    bool kppaattyy = kp()->tilikaudet()->kirjanpitoLoppuu() == tilikausi.paattyy();

    ui->eiJaksotettavaaLabel->setVisible( !jaksotuksetkirjattu && eijaksotuksia);
    ui->jaksotTehty->setVisible( jaksotuksetkirjattu );
    ui->jaksotKirjattuLabel->setVisible( jaksotuksetkirjattu );
    ui->jaksotusNappi->setVisible(!jaksotuksetkirjattu && !eijaksotuksia && !kppaattyy);
    ui->jaksoStop->setVisible(kppaattyy && !eijaksotuksia);
    ui->jaksoAvaa->setVisible(kppaattyy && !eijaksotuksia);


    bool verokirjattu = data_.value("tulovero").toString() == "kirjattu";
    bool eiverotettavaa = qAbs(data_.value("tulovero").toMap().value("tulo").toDouble()) < 1e-3;
    const QString& muoto = kp()->asetukset()->asetus(AsetusModel::Muoto);
    bool vaaramuoto = muoto == "ay" || muoto == "ky"
            || muoto == "tmi";

    if( vaaramuoto )
        ui->eiveroaLabel->setText(tr("Tuloverotus yrittäjän verotuksessa"));
    else if( eiverotettavaa)
        ui->eiveroaLabel->setText(tr("Ei verotettavaa tuloverossa"));
    ui->eiveroaLabel->setVisible(!verokirjattu && (vaaramuoto || eiverotettavaa));
    ui->veroTehty->setVisible(verokirjattu);
    ui->veroKirjattuLabel->setVisible(verokirjattu);
    ui->tuloveroNappi->setVisible(!verokirjattu && !eiverotettavaa && !vaaramuoto);

    const int tilioimattomia = data_.value("tilioimatta").toInt();
    ui->tilioimattaFrame->setVisible(tilioimattomia);
    if( tilioimattomia ) {
        ui->lukitseNappi->setEnabled(false);
    }

    ui->yksityistiliGroup->setVisible( muoto == "tmi" );
    const bool yksityistilitPaatetty = data_.value("yksityistilit").toString() == "kirjattu";
    bool lukittu = kp()->tilitpaatetty() >= tilikausi.paattyy();

    ui->yksityistiliCheck->setVisible(yksityistilitPaatetty);
    ui->yksityistiliTehtyLabel->setVisible(yksityistilitPaatetty);
    ui->yksityistiliButton->setVisible(!yksityistilitPaatetty);
    ui->yksityistiliButton->setEnabled(!kppaattyy && lukittu);
    ui->yksityistilitStop->setVisible(kppaattyy);
    ui->yksityistilitStopLabel->setVisible(kppaattyy);
    if( muoto == "tmi") {
        ui->vahvistaLabel->setText(tr("11. Lukitse tilinpäätös"));
    }

}
