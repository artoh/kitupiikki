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
#include "tuloverodialog.h"
#include "ui_tuloverodialog.h"

#include "db/kirjanpito.h"
#include "model/tosite.h"
#include "model/tositevienti.h"
#include "model/tositeviennit.h"
#include "liite/liitteetmodel.h"
#include "db/tositetyyppimodel.h"
#include <QPushButton>
#include <QMessageBox>
#include <QSettings>

TuloveroDialog::TuloveroDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TuloveroDialog)
{
    ui->setupUi(this);   


    connect( ui->tuloEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaTulos);
    connect( ui->vahennysEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaTulos);

    connect( ui->tulosEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaLopullinenTulos);
    connect( ui->tappioEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaLopullinenTulos);

    connect( ui->loppuTulosEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaVero);

    connect( ui->veroEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaJaannos);
    connect( ui->maksetutEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaJaannos);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("tilikaudet/tilinpaatos/tulovero/");});

    setAttribute(Qt::WA_DeleteOnClose);

    restoreGeometry( kp()->settings()->value("TuloveroDialog").toByteArray() );
}

TuloveroDialog::~TuloveroDialog()
{
    kp()->settings()->setValue("TuloveroDialog", saveGeometry());

    delete ui;
}

void TuloveroDialog::alusta(const QVariantMap &verolaskelma, const Tilikausi &tilikausi)
{
    tilikausi_ = tilikausi;

    ui->tuloEdit->setEuro( verolaskelma.value("tulo").toString());
    ui->vahennysEdit->setEuro( verolaskelma.value("vahennys").toString());
    ui->maksetutEdit->setEuro( verolaskelma.value("ennakko").toString());

    paivitaTulos();

}

void TuloveroDialog::accept()
{
    if( !ui->jaaveroaEdit->asCents()) {

        QDialog::accept();
        QMessageBox::information(this, tr("Tuloveron kirjaaminen"),
                                 tr("Ennakkovero täsmää täysin tuloveroon, eikä tuloveroa jää "
                                    "myöhemmin tilitettäväksi.\n"
                                    "Tallenna veroilmoituksesi osaksi kirjanpitoa."));
        return;
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    Tosite* tosite = new Tosite(this);
    tosite->asetaTyyppi(TositeTyyppi::TULOVERO);
    tosite->asetaPvm( tilikausi_.paattyy());
    tosite->asetaOtsikko(tr("Tuloveron jaksotus tilikaudelta %1").arg(tilikausi_.kausivaliTekstina()));
    tosite->asetaSarja( kp()->tositeTyypit()->sarja( TositeTyyppi::TULOVERO ) ) ;

    QString selite = tr("Tuloveron jaksotus tilikaudelta %1").arg(tilikausi_.kausivaliTekstina());

    TositeVienti jaksovienti;
    jaksovienti.setPvm(tilikausi_.paattyy());
    jaksovienti.setTili(kp()->asetukset()->luku("Tuloverojaksotustili", 9940));
    if( ui->jaaveroaEdit->asCents() > 0)
        jaksovienti.setDebet(ui->jaaveroaEdit->asCents());
    else
        jaksovienti.setKredit(0 - ui->jaaveroaEdit->asCents());
    jaksovienti.setJaksoalkaa( tilikausi_.alkaa() );
    jaksovienti.setJaksoloppuu( tilikausi_.paattyy());
    jaksovienti.setJaksotustili( kp()->asetukset()->luku("Tuloverojaksotasetili", 2981) );
    jaksovienti.setSelite(selite);
    tosite->viennit()->lisaa(jaksovienti);

    TositeVienti siirtovienti;
    siirtovienti.setPvm(tilikausi_.paattyy());
    siirtovienti.setEra(-1);        // Oma tase-eränsä
    if( ui->jaaveroaEdit->asCents() > 0) {
        siirtovienti.setTili(kp()->asetukset()->luku("Tuloverosiirtovelat", 2968));
        siirtovienti.setKredit(ui->jaaveroaEdit->asCents());
    } else {
        siirtovienti.setTili(kp()->asetukset()->luku("Tuloverosiirtosaamiset", 1813));
        siirtovienti.setDebet(0 - ui->jaaveroaEdit->asCents());
    }
    siirtovienti.setSelite(selite);
    tosite->viennit()->lisaa(siirtovienti);
    tosite->liitteet()->lisaa( liite(), "verolaskelma.pdf", "verolaskelma" );


    connect( tosite, &Tosite::talletettu, this, &TuloveroDialog::kirjattu);
    tosite->tallenna();
}


void TuloveroDialog::paivitaTulos()
{
    ui->tulosEdit->setValue( ui->tuloEdit->value() - ui->vahennysEdit->value());
    paivitaLopullinenTulos();
}


void TuloveroDialog::paivitaLopullinenTulos()
{
    ui->loppuTulosEdit->setValue(ui->tulosEdit->value() - ui->tappioEdit->value());
    paivitaVero();    
}


void TuloveroDialog::paivitaVero()
{
    double tulos = ui->loppuTulosEdit->value();

    if( tulos > 1e-5)
        ui->veroEdit->setValue(0.2 * tulos);
    else
        ui->veroEdit->setValue(0);
    paivitaJaannos();
}

void TuloveroDialog::paivitaJaannos()
{
    ui->jaaveroaEdit->setValue( ui->veroEdit->value() - ui->maksetutEdit->value());
}

void TuloveroDialog::kirjattu()
{
    QDialog::accept();
    QMessageBox::information(this, tr("Tuloveron kirjaus tallennettu"),
                             tr("Tuloverot on kirjattu.\n"
                                "Säilytä veroilmoitus ja mahdolliset verolaskelmasi "
                                "kirjanpitosi yhteydessä."));
    emit tallennettu();
}

QByteArray TuloveroDialog::liite() const
{
    RaportinKirjoittaja rk;
    rk.asetaOtsikko( tr("Tuloverolaskelma") );
    rk.asetaKausiteksti( tilikausi_.kausivaliTekstina() );

    rk.lisaaVenyvaSarake();
    rk.lisaaEurosarake();
    rk.lisaaTyhjaRivi();

    rivi(rk, tr("Veronalainen tulo yhteensä"), ui->tuloEdit->euro());
    rivi(rk, tr("Vähennyskelpoiset kulut"), ui->vahennysEdit->euro());
    rk.lisaaTyhjaRivi();
    rivi(rk, tr("Verotettava tulos"), ui->tulosEdit->euro());
    rivi(rk, tr("Vähennettävä aiempi tappio"), ui->tappioEdit->euro());
    rk.lisaaTyhjaRivi();
    rivi(rk, tr("Lopullinen verotettava tulos"), ui->loppuTulosEdit->euro());
    rivi(rk, tr("Tuloveron määrä"), ui->veroEdit->euro());
    rk.lisaaTyhjaRivi();
    rivi(rk, tr("Maksetut tuloverot"), ui->maksetutEdit->euro());
    rivi(rk, tr("Maksamaton tulovero"), ui->jaaveroaEdit->euro());

    return rk.pdf(false, true);
}

void TuloveroDialog::rivi(RaportinKirjoittaja &rk, const QString &teksti, const Euro &maara)
{
    RaporttiRivi rivi;
    rivi.lisaa( teksti );
    rivi.lisaa( maara, true);
    rk.lisaaRivi(rivi);
}
