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

#include <QPushButton>
#include <QTextDocument>
#include <QMessageBox>
#include "alvilmoitusdialog.h"
#include "ui_alvilmoitusdialog.h"

#include "db/kirjanpito.h"

#include "naytin/naytinikkuna.h"

#include "alvlaskelma.h"
#include "alvilmoitustenmodel.h"
#include "alvkaudet.h"

#include <QRegularExpressionValidator>
#include <QMessageBox>

AlvIlmoitusDialog::AlvIlmoitusDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AlvIlmoitusDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("alv/ilmoitus/");});

    ui->puhelinEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\+\\d+")));

    connect( ui->yhteysEdit, &QLineEdit::textEdited, this, &AlvIlmoitusDialog::tarkastaKelpo);
    connect( ui->puhelinEdit, &QLineEdit::textEdited, this, &AlvIlmoitusDialog::tarkastaKelpo);
    connect( ui->ilmoitaGroup, &QGroupBox::toggled, this, &AlvIlmoitusDialog::tarkastaKelpo);

    connect( ui->alaLvEdit, &KpEuroEdit::textEdited, this, &AlvIlmoitusDialog::paivitaHuojennus);
    connect( ui->alaVeroEdit, &KpEuroEdit::textEdited, this, &AlvIlmoitusDialog::paivitaHuojennus);


    ui->korjausCombo->addItem(tr("Laskuvirhe"), "CLC");
    ui->korjausCombo->addItem(tr("Oikeuskäytännön muutos"),"LGL");
    ui->korjausCombo->addItem(tr("Verotarkastuksessa saatu ohjaus"),"TXA");
    ui->korjausCombo->addItem(tr("Laintulkintavirhe"), "LAW");
}

AlvIlmoitusDialog::~AlvIlmoitusDialog()
{
    delete ui;
}


void AlvIlmoitusDialog::accept()
{
    kp()->asetukset()->aseta(AsetusModel::VeroYhteysHenkilo, ui->yhteysEdit->text());
    kp()->asetukset()->aseta(AsetusModel::VeroYhteysPuhelin, ui->puhelinEdit->text());

    bool huojennusRuksattu = ui->alarajaGroup->isChecked();

    // Korjataan huojennukseen syötetyt lukemat. Jos ei ruksattu, nollat
    laskelma_->korjaaHuojennus( huojennusRuksattu ? ui->alaLvEdit->euro() : Euro::Zero,
                                huojennusRuksattu ? ui->alaVeroEdit->euro() : Euro::Zero);
    laskelma_->kirjoitaLaskelma();
    laskelma_->kirjaaVerot();
    laskelma_->kirjaaHuojennus();

    laskelma_->valmisteleTosite();    

    connect( laskelma_, &AlvLaskelma::tallennettu, this, &AlvIlmoitusDialog::laskemaTallennettu);
    connect( laskelma_, &AlvLaskelma::ilmoitusVirhe, this, &AlvIlmoitusDialog::ilmoitusVirhe);


    if( ui->ilmoitaGroup->isChecked() ) {
        laskelma_->ilmoitaJaTallenna( ui->korjausCombo->isVisible() ? ui->korjausCombo->currentData().toString() : QString());
    } else {
        laskelma_->tallenna();
    }
    kp()->odotusKursori(true);
    ui->buttonBox->setEnabled(false);

    qApp->processEvents();   
}

void AlvIlmoitusDialog::reject()
{
    laskelma_->deleteLater();
    QDialog::reject();
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

void AlvIlmoitusDialog::tarkastaKelpo()
{
    bool kelpo =
            !ui->ilmoitaGroup->isChecked() ||
            ( ui->yhteysEdit->text().length() > 4 &&
            ui->puhelinEdit->text().length() > 8 );
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(kelpo);
}

void AlvIlmoitusDialog::paivitaHuojennus()
{
    Euro huojennus = laskelma_->huojennuksenMaara(ui->alaLvEdit->euro(), ui->alaVeroEdit->euro());
    if( huojennus ) {
        ui->maaraLabel->setText( huojennus.display(true));
    } else {
        ui->maaraLabel->setText( tr("Ei oikeuta alarajahuojennukseen"));
    }

}

void AlvIlmoitusDialog::ilmoitusVirhe(const QString &koodi, const QString &viesti)
{
    kp()->odotusKursori(false);
    QMessageBox::critical(this, tr("Virhe ilmoittamisessa"),
                          tr("Alv-ilmoituksen toimittaminen verottajalle epäonnistui.\n") +
                          QString("%1\n(%2)").arg( viesti, koodi ) );
    ui->buttonBox->setEnabled(true);
}


void AlvIlmoitusDialog::naytaLaskelma(RaportinKirjoittaja rk)
{
    laskelma_ = qobject_cast<AlvLaskelma*>( sender() );
    ui->ilmoitusBrowser->setHtml( rk.html() );

    ui->alarajaGroup->setVisible( laskelma_->huojennusAika() );
    ui->alarajaGroup->setChecked( laskelma_->huojennus());
    ui->alaLvEdit->setEuro( laskelma_->liikevaihto() );
    ui->alaVeroEdit->setEuro( laskelma_->verohuojennukseen() );
    if( laskelma_->liikevaihto()) paivitaHuojennus();

    QPushButton* avaa = ui->buttonBox->addButton(tr("Tulosta"), QDialogButtonBox::ApplyRole);
    avaa->setIcon(QIcon(":/pic/print.png"));
    connect( avaa, &QPushButton::clicked, [rk] {NaytinIkkuna::naytaRaportti(rk);});

    bool ilmoitusKaytossa = kp()->alvIlmoitukset()->kaudet()->alvIlmoitusKaytossa() &&
                            !kp()->onkoHarjoitus();

    ui->ilmoitaGroup->setVisible( ilmoitusKaytossa );
    ui->ilmoitaGroup->setChecked( ilmoitusKaytossa );

    AlvKausi kausi = kp()->alvIlmoitukset()->kaudet()->kausi( laskelma_->loppupvm() );
    bool korjaus = kausi.tila() == AlvKausi::KASITELTY || kausi.tila() == AlvKausi::KASITTELYSSA;

    ui->korjausCombo->setVisible(korjaus);
    ui->korjausLabel->setVisible(korjaus);
    ui->yhteysEdit->setText(  kp()->asetukset()->asetus(AsetusModel::VeroYhteysHenkilo) );
    ui->puhelinEdit->setText( kp()->asetukset()->asetus(AsetusModel::VeroYhteysPuhelin));
    if( ui->puhelinEdit->text().isEmpty())
        ui->puhelinEdit->setText("+358");
    tarkastaKelpo();

    show();

}

void AlvIlmoitusDialog::laskemaTallennettu()
{
    kp()->odotusKursori(false);
    laskelma_->deleteLater();
    QDialog::accept();
}

