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
#include "yhteystietowidget.h"

#include "ui_yhteystiedot.h"
#include "validator/ytunnusvalidator.h"
#include "db/jsonkentta.h"

#include <QSqlQuery>

#include <QDebug>
#include <QSqlError>

YhteystietoWidget::YhteystietoWidget(QWidget *parent) : QWidget(parent), ui_( new Ui::Yhteystiedot)
{
    ui_->setupUi(this);
    ui_->YtunnusEdit->setValidator(new YTunnusValidator);
    ui_->tallennaNappi->setEnabled(false);

    connect( ui_->osoiteEdit, &QPlainTextEdit::textChanged, this, &YhteystietoWidget::muokattu);
    connect( ui_->spostiEdit, &QLineEdit::textChanged, this, &YhteystietoWidget::muokattu);
    connect(ui_->YtunnusEdit, &QLineEdit::textChanged, this, &YhteystietoWidget::muokattu);
    connect(ui_->tallennaNappi, &QPushButton::clicked, this, &YhteystietoWidget::tallenna);
    connect(ui_->peruNappi, &QPushButton::clicked, this, &YhteystietoWidget::nollaa);
    connect(ui_->nimiEdit, &QLineEdit::textEdited, this, &YhteystietoWidget::taydennaOsoite);
}

YhteystietoWidget::~YhteystietoWidget()
{
    delete ui_;
}

void YhteystietoWidget::haeTiedot(const QString &nimi)
{
    nimi_ = nimi;
    osoite_ = nimi + "\n";
    sahkoposti_.clear();
    ytunnus_.clear();
    verkkolaskuosoite_.clear();
    verkkolaskuvalittaja_.clear();

    if( !nimi.isEmpty())
    {

        QSqlQuery kysely;
        kysely.exec( QString("SELECT json FROM vienti WHERE asiakas='%1' AND iban is null ORDER BY muokattu DESC").arg( nimi_ )) ;
        if( kysely.next())
        {
            JsonKentta json;
            json.fromJson( kysely.value(0).toByteArray());
            if( !json.str("Osoite").isEmpty())
                osoite_ = json.str("Osoite");
            sahkoposti_ = json.str("Email");
            ytunnus_ = json.str("YTunnus");
            verkkolaskuosoite_ = json.str("VerkkolaskuOsoite");
            verkkolaskuvalittaja_ = json.str("VerkkolaskuValittaja");
        }
    }

    nollaa();
}

void YhteystietoWidget::taydennaOsoite()
{
    QString jatke = ui_->osoiteEdit->toPlainText().mid( ui_->osoiteEdit->toPlainText().indexOf('\n')+1 );
    ui_->osoiteEdit->setPlainText( ui_->nimiEdit->text() + "\n" + jatke );
}

void YhteystietoWidget::tallenna()
{
    bool uusi = nimi_.isEmpty();
    nimi_ = ui_->nimiEdit->text();
    osoite_ = ui_->osoiteEdit->toPlainText();
    sahkoposti_ = ui_->spostiEdit->text();
    ytunnus_ = ui_->YtunnusEdit->text();
    verkkolaskuosoite_ = ui_->verkkolaskuOsoite->text();
    verkkolaskuvalittaja_ = ui_->valittajaTunnus->text();

    JsonKentta json;
    if( !osoite_.isEmpty())
        json.set("Osoite", osoite_);
    if( !sahkoposti_.isEmpty())
        json.set("Email", sahkoposti_);
    if( !ytunnus_.isEmpty())
        json.set("YTunnus", ytunnus_);
    if( !verkkolaskuosoite_.isEmpty())
        json.set("VerkkolaskuOsoite", verkkolaskuosoite_);
    if( !verkkolaskuvalittaja_.isEmpty())
        json.set("VerkkolaskuValittaja", verkkolaskuvalittaja_);


    QSqlQuery kysely;
    kysely.prepare("INSERT INTO vienti (vientirivi, asiakas, json, luotu, muokattu) "
                   "VALUES (0, :asiakas, :json, :luotu, :muokattu)");
    kysely.bindValue(":asiakas", nimi_);
    kysely.bindValue(":json", json.toSqlJson());
    kysely.bindValue(":luotu", QDateTime::currentDateTime());
    kysely.bindValue(":muokattu", QDateTime::currentDateTime());

    kysely.exec();

    ui_->tallennaNappi->setEnabled(false);
    ui_->nimiEdit->setEnabled(false);

    if( uusi )
        emit uusiAsiakas(nimi_);
}

void YhteystietoWidget::muokattu()
{
    ui_->tallennaNappi->setEnabled( ui_->osoiteEdit->toPlainText() != osoite_ ||
            ui_->spostiEdit->text() != sahkoposti_ ||
            ui_->YtunnusEdit->text() != ytunnus_ );
}

void YhteystietoWidget::nollaa()
{
    ui_->nimiEdit->setText( nimi_ );
    ui_->nimiEdit->setEnabled( nimi_.isEmpty() );

    ui_->osoiteEdit->setPlainText(osoite_);
    ui_->spostiEdit->setText(sahkoposti_);
    ui_->YtunnusEdit->setText(ytunnus_);
    ui_->verkkolaskuOsoite->setText( verkkolaskuosoite_);
    ui_->valittajaTunnus->setText( verkkolaskuvalittaja_);

    ui_->tallennaNappi->setEnabled(false);
}

