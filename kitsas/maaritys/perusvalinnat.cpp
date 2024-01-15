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

#include <QFileDialog>
#include <QFile>
#include <QImage>
#include <QPixmap>
#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>

#include "perusvalinnat.h"
#include "ui_perusvalinnat.h"
#include "arkistoija/arkistohakemistodialogi.h"

#include "db/kirjanpito.h"
#include "sqlite/sqlitemodel.h"
#include "pilvi/pilvimodel.h"
#include "validator/ytunnusvalidator.h"

#include "kieli/kielet.h"
#include "extra/aliasdialog.h"

Perusvalinnat::Perusvalinnat() :
    TallentavaMaaritysWidget(nullptr),
    ui(new Ui::Perusvalinnat)
{
    ui->setupUi(this);    

    connect( ui->hakemistoNappi, &QPushButton::clicked, this, &Perusvalinnat::avaaHakemisto );
    connect( ui->avaaArkistoNappi, &QPushButton::clicked, this, [this] { kp()->avaaUrl( QUrl("file://" + ui->arkistoEdit->text(), QUrl::TolerantMode) ); });
    connect( ui->vaihdaArkistoNappi, &QPushButton::clicked, this, &Perusvalinnat::vaihdaArkistoHakemisto);
    connect( ui->harjoitusCheck, &QPushButton::clicked, this, &Perusvalinnat::naytaVastuu);
    connect( ui->aliasButton, &QPushButton::clicked, this, &Perusvalinnat::vaihdaAlias);

    ui->ytunnusEdit->setValidator(new YTunnusValidator());

}

Perusvalinnat::~Perusvalinnat()
{
    delete ui;
}

bool Perusvalinnat::nollaa()
{
    TallentavaMaaritysWidget::nollaa();

    connect( ui->laajuusCombo, &QComboBox::currentTextChanged, this, &Perusvalinnat::alvilaajuudesta);

    SQLiteModel *sqlite = qobject_cast<SQLiteModel*>( kp()->yhteysModel() );
    if( sqlite ) {
        ui->sijaintiLabel->setText( sqlite->tiedostopolku() );

        ui->aliasLabel->hide();
        ui->aliasEdit->hide();
        ui->aliasButton->hide();
    } else {
        ui->sijaintiLabel->hide();
        ui->hakemistoNappi->hide();
        ui->tsLabel->hide();

        ui->aliasLabel->hide();
        ui->aliasEdit->hide();
        ui->aliasButton->hide();

        // Alias toistaiseksi poissa käytöstä
        // ui->aliasEdit->setText( kp()->pilvi()->pilvi().alias() );
    }    

    ui->karttaInfo->setText( QString("%1 %2")
                             .arg(kp()->asetukset()->asetus(AsetusModel::Tilikartta), kp()->asetukset()->pvm(AsetusModel::TilikarttaPvm).toString("dd.MM.yyyy")));
    ui->alvAlkaaEdit->setEnabled( ui->alvCheck->isChecked());

    ui->arkistoEdit->setText(kp()->settings()->value("arkistopolku/" + kp()->asetukset()->uid()).toString());
    ui->avaaArkistoNappi->setEnabled( !ui->arkistoEdit->text().isEmpty() );

    KpKysely* kokokysely = kpk("/info");
    connect(kokokysely, &KpKysely::vastaus, this, &Perusvalinnat::kokoSaapuu);
    kokokysely->kysy();

    ui->alvCheck->setEnabled(qobject_cast<PilviModel*>(kp()->yhteysModel()) == nullptr || kp()->pilvi()->pilvi().vat() ||
                             kp()->asetukset()->onko(AsetusModel::AlvVelvollinen));


    return true;
}

bool Perusvalinnat::onkoMuokattu()
{
    return TallentavaMaaritysWidget::onkoMuokattu() ;
}


bool Perusvalinnat::tallenna()
{
    ui->ytunnusEdit->setText(ui->ytunnusEdit->text().simplified());    

    // Jos muoto tai laajuus vaihtuu, vaikuttaa se tilikarttaan ja ehkä myös alviin
    TallentavaMaaritysWidget::tallenna();
    emit kp()->perusAsetusMuuttui();     // Uusi lataus, koska nimi tai kuva saattoi vaihtua!    
    kp()->tilit()->paivitaTilat();        

    return true;
}

void Perusvalinnat::alvilaajuudesta()
{
    int laajuus = ui->laajuusCombo->currentData().toInt();
    int alvlaajuus = kp()->asetukset()->luku("alvlaajuus");
    ui->alvCheck->setChecked( laajuus >= alvlaajuus );
}



void Perusvalinnat::naytaVastuu(bool harjoitus)
{
    if( !harjoitus) {
        QMessageBox::warning(this,
                             tr("Vastuu kirjanpidosta"),
                             tr("Olet itse vastuussa kirjanpitosi oikeellisuudesta ja laillisuudesta sekä siitä, että kaikki verot maksetaan asianmukaisesti.\n\n"
                                "Ohjelmalla ei ole mitään takuuta. Kitsas Oy ei myöskään anna oikeudellista neuvontaa kirjanpidosta tai verotuksesta.\n\n"
                                "Käänny tarvittaessa kirjanpidon ammattilaisen puoleen"));
    }
}

void Perusvalinnat::vaihdaAlias()
{
    AliasDialog dlg(this);
    dlg.asetaAlias(kp()->pilvi()->pilvi().alias());
    if( dlg.exec() == QDialog::Accepted) {
        const QString& alias = dlg.alias();
        kp()->pilvi()->asetaAlias(alias);
        ui->aliasEdit->setText(alias);
    }
}

void Perusvalinnat::avaaHakemisto()
{
    QFileInfo info( ui->sijaintiLabel->text() );
    kp()->avaaUrl( QUrl("file://" + info.dir().absolutePath(), QUrl::TolerantMode));
}

void Perusvalinnat::vaihdaArkistoHakemisto()
{
    ui->arkistoEdit->setText( ArkistohakemistoDialogi::valitseArkistoHakemisto(this) );
    ui->avaaArkistoNappi->setEnabled( !ui->arkistoEdit->text().isEmpty());
}

void Perusvalinnat::kokoSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    long koko = map.value("koko").toInt();
    double megaa = koko / 1e6;
    ui->kokoEdit->setText(tr("%L1 Mt").arg(megaa,0,'f',1));

}
