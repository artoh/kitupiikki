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
#include "db/tositetyyppimodel.h"
#include <QPushButton>
#include <QMessageBox>

TuloveroDialog::TuloveroDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TuloveroDialog)
{
    ui->setupUi(this);   


    connect( ui->tuloEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaTulos);
    connect( ui->vahennysEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaTulos);

    connect( ui->tulosEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaEnnenYlea);
    connect( ui->tappioEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaEnnenYlea);

    connect( ui->ennenYlea, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaYlevero);
    connect( ui->yleveroEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaYlenjalkeen);

    connect( ui->ylenJalkeen, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaVero);

    connect( ui->veroEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaJaannos);
    connect( ui->maksetutEdit, &KpEuroEdit::textEdited, this, &TuloveroDialog::paivitaJaannos);

    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("tilinpaatos/tulovero");});

    setAttribute(Qt::WA_DeleteOnClose);
}

TuloveroDialog::~TuloveroDialog()
{
    delete ui;
}

void TuloveroDialog::alusta(const QVariantMap &verolaskelma, const Tilikausi &tilikausi)
{
    tilikausi_ = tilikausi;

    ui->tuloEdit->setValue( verolaskelma.value("tulo").toDouble());
    ui->vahennysEdit->setValue( verolaskelma.value("vahennys").toDouble());
    ui->maksetutEdit->setValue( verolaskelma.value("ennakko").toDouble());

    paivitaTulos();

}

void TuloveroDialog::accept()
{
    if( !ui->yleveroEdit->asCents() && !ui->jaaveroaEdit->asCents()) {

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

    if( ui->yleveroEdit->value() > 1e-5) {
        QString yleselite = tr("Ylevero tilikaudelta %1").arg(tilikausi_.kausivaliTekstina());

        TositeVienti yledebet;
        yledebet.setPvm(tilikausi_.paattyy());
        yledebet.setTili( kp()->asetukset()->luku("Yleverotili", 8740) );
        yledebet.setDebet(ui->yleveroEdit->value());
        yledebet.setSelite(yleselite);
        tosite->viennit()->lisaa(yledebet);

        TositeVienti ylekredit;
        ylekredit.setPvm(tilikausi_.paattyy());
        ylekredit.setTili( kp()->asetukset()->luku("Tuloverojaksotustili",9940));
        ylekredit.setKredit( ui->yleveroEdit->value());
        ylekredit.setSelite(yleselite);
        tosite->viennit()->lisaa(ylekredit);
    }
    QString selite = tr("Tuloveron jaksotus tilikaudelta %1").arg(tilikausi_.kausivaliTekstina());

    TositeVienti jaksovienti;
    jaksovienti.setPvm(tilikausi_.paattyy());
    jaksovienti.setTili(kp()->asetukset()->luku("Tuloverojaksotustili", 9940));
    if( ui->jaaveroaEdit->asCents() > 0)
        jaksovienti.setDebet(ui->jaaveroaEdit->asCents());
    else
        jaksovienti.setKredit(0 - ui->jaaveroaEdit->asCents());
    jaksovienti.setSelite(selite);
    tosite->viennit()->lisaa(jaksovienti);

    TositeVienti siirtovienti;
    siirtovienti.setPvm(tilikausi_.paattyy());
    siirtovienti.setEra(-1);        // Oma tase-eränsä
    if( ui->jaaveroaEdit->asCents() > 0) {
        siirtovienti.setTili(kp()->asetukset()->luku("Tuloverosiirtovelat",2968));
        siirtovienti.setKredit(ui->jaaveroaEdit->asCents());
    } else {
        siirtovienti.setTili(kp()->asetukset()->luku("Tuloverosiirtosaamiset",1813));
        siirtovienti.setDebet(0 - ui->jaaveroaEdit->asCents());
    }
    siirtovienti.setSelite(selite);
    tosite->viennit()->lisaa(siirtovienti);

    connect( tosite, &Tosite::talletettu, this, &TuloveroDialog::kirjattu);
    tosite->tallenna();
}


void TuloveroDialog::paivitaTulos()
{
    ui->tulosEdit->setValue( ui->tuloEdit->value() - ui->vahennysEdit->value());
    paivitaEnnenYlea();
}

void TuloveroDialog::paivitaEnnenYlea()
{
    ui->ennenYlea->setValue(ui->tulosEdit->value() - ui->tappioEdit->value());
    paivitaYlevero();
}

void TuloveroDialog::paivitaYlevero()
{
    double tulo = ui->ennenYlea->value();

    if( tulo < 50000)
        ui->yleveroEdit->setValue(0);
    else if( tulo >= 867142)
        ui->yleveroEdit->setValue(3000);
    else
        ui->yleveroEdit->setValue(140+(0.0035*(tulo - 50000)));
    paivitaYlenjalkeen();
}

void TuloveroDialog::paivitaYlenjalkeen()
{
    ui->ylenJalkeen->setValue(ui->ennenYlea->value() - ui->yleveroEdit->value());
    paivitaVero();
}


void TuloveroDialog::paivitaVero()
{
    double tulos = ui->ylenJalkeen->value();

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
