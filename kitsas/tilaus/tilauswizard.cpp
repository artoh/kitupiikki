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
#include "tilauswizard.h"

#include "planmodel.h"
#include "tilausvalintasivu.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include <QMessageBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>

#include "validator/ytunnusvalidator.h"
#include "rekisteri/postinumerot.h"
#include "rekisteri/asiakastoimittajadlg.h"


#include "ui_tilausyhteys.h"
#include "ui_tilausvahvistus.h"
#include "ui_kiitos.h"

TilausWizard::TilausWizard() :
    planModel_( new PlanModel(this)),
    valintaSivu_( new TilausValintaSivu(planModel_))
{
    addPage( valintaSivu_ );
    addPage( new TilausYhteysSivu);
    addPage( new TilausVahvistusSivu(this));
}

int TilausWizard::nextId() const
{
    // Jos valitaan maksuton plan, siihen ei tarvita laskutustietoja
    if( currentId() == VALINTA && !valintaSivu_->tilaus(PlanModel::PlanRooli).toInt())
        return VAHVISTUS;
    return QWizard::nextId();
}

void TilausWizard::nayta()
{
    QNetworkRequest request( QUrl( kp()->pilvi()->pilviLoginOsoite() + "/subscription" ));
    request.setRawHeader("Authorization", QString("bearer %1").arg( kp()->pilvi()->token() ).toLatin1());
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
    QNetworkReply *reply = kp()->networkManager()->get(request);
    connect( reply, &QNetworkReply::finished, this, &TilausWizard::dataSaapuu);
}

QString TilausWizard::yhteenveto()
{
    QString txt = tr("Tilaamasi paketti: %1 \n\n").arg(valintaSivu_->tilaus(PlanModel::NimiRooli).toString());

    txt.append(valintaSivu_->tilaus(PlanModel::InfoRooli).toString());

    bool joustavatLisaPilvet = valintaSivu_->tilaus(PlanModel::LisaPilviKkHinta ).isValid();

    if( joustavatLisaPilvet ) {
        Euro lisahinta = Euro::fromVariant(valintaSivu_->tilaus(PlanModel::LisaPilviKkHinta));
        if( valintaSivu_->tilaus(PlanModel::PilviaRooli ).toInt() > 1 ) {
            txt.append("\n" + tr("Paketin hintaan sisältyy %1 kirjanpitoa.").arg(valintaSivu_->tilaus(PlanModel::PilviaRooli ).toInt()) + " ");
        }
        txt.append( "\n" + tr("Lisäkirjanpidoista laskutetaan jälkikäteen %1/kk.").arg(lisahinta.display()) );
    } else {
        txt.append( tr("\nVoit tallentaa pilveen enintään %1 kirjanpitoa.")
                    .arg( valintaSivu_->tilaus(PlanModel::PilviaRooli ).toInt() + field("lisapilvet").toInt() ) );
    }

    if( valintaSivu_->tilaus(PlanModel::PlanRooli).toInt() ) {

        double hinta = valintaSivu_->tilaus(PlanModel::HintaRooli).toDouble() +
                (joustavatLisaPilvet ? 0 : field("lisapilvet").toInt() * valintaSivu_->tilaus(PlanModel::LisaPilviHinta).toDouble() );

        txt.append( tr("\n\nHinta %L1 € ").arg( hinta ,0,'f',2 ));
        if( field("puolivuosittain").toBool())
            txt.append( tr("laskutettuna puolivuosittain\n") );
        else
            txt.append( tr("laskutettuna vuosittain\n"));

        double hyvitys = current_.value("refund").toDouble();
        if( hyvitys > hinta)
            hyvitys = hinta;

        if( hyvitys > 1) {
            txt.append(tr("Ensimmäiseltä laskulta vähennetään nykyistä tilaustasi %L1€, "
                       "jolloin maksettavaa jää %L2€.")
                    .arg(hyvitys,0,'f',2)
                    .arg(hinta - hyvitys, 0, 'f', 2) );
        }

        if(field("sahkopostilla").toBool()) {
            txt.append(tr("\nLasku toimitetaan sähköpostilla osoitteeseen %1")
                   .arg( field("email").toString() ));
        } else {
            txt.append(tr("\nLasku toimitetaan verkkolaskuna"));
        }

        txt.append(tr("\n\nLaskun saajaksi merkitään\n"));
        txt.append( field("name").toString() + "\n");
        txt.append( field("address").toString() + "\n");
        txt.append( field("postcode").toString() + " ");
        txt.append( field("town").toString() + "\n");

        if( !field("asviite").toString().isEmpty())
            txt.append("\nAsiakkaan viite: " + field("asviite").toString());
    }

    return txt;

}

void TilausWizard::dataSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error() ) {
        qDebug() << reply->readAll();
        QMessageBox::critical(nullptr, tr("Verkkovirhe"), tr("Tilaustietojen hakeminen palvelimelta epäonnistui"));
        deleteLater();
        return;
    }
    QVariantMap map = QJsonDocument::fromJson(reply->readAll()).toVariant().toMap();

    current_ = map.value("current").toMap();

    int nykyplan = current_.value("plan").toInt();

    if( nykyplan >= 100) {
        QMessageBox::information(nullptr, tr("VIP-asiakas"),
                                 tr("Sinulla on voimassa oleva VIP-asiakkuus. \n\n"
                                    "Muutokset tehdään asiakaspalvelun kautta."));
        deleteLater();
        return;
    }


    if( current_.value("email").toString().isEmpty())
        setField("email", kp()->pilvi()->kayttajaEmail());
    else
        setField("email", current_.value("email"));

    bool puolivuosittain = current_.value("months").toInt() == 6;

    QVariantMap pinfo = current_.value("payer").toMap();
    setField("name", pinfo.value("name").toString());
    setField("address", pinfo.value("address"));
    setField("postcode", pinfo.value("postcode"));
    setField("town", pinfo.value("town"));
    setField("asviite", pinfo.value("customerref"));
    setField("phone", pinfo.value("phone"));

    setField("ovt", pinfo.value("ovt"));
    setField("operator", pinfo.value("operator"));


    setField("sahkopostilla", pinfo.value("operator").toString().isEmpty());
    setField("verkkolaskulla", !pinfo.value("operator").toString().isEmpty());

    if(pinfo.contains("vatnumber"))
    setField("ytunnus", AsiakasToimittajaDlg::alvToY( pinfo.value("vatnumber").toString()));

    planModel_->alusta( map.value("plans").toList(), puolivuosittain);
    valintaSivu_->alusta(nykyplan, puolivuosittain,
                         current_.value("refund").toDouble(),
                         map.value("cloudgigas").toDouble(),
                         current_.value("extraclouds").toInt());

    if( field("ovt").toString().isEmpty() && !field("ytunnus").toString().isEmpty()) {
        setField("ovt", "0037" + field("ytunnus").toString().remove('-'));
    }


    if( exec() )
    {
        QVariantMap tmap;
        int plan = valintaSivu_->tilaus(PlanModel::PlanRooli).toInt();
        tmap.insert("plan", plan);
        tmap.insert("extraclouds", field("lisapilvet"));

        if( plan) {
            tmap.insert("period", field("puolivuosittain").toBool() ? 6 : 12 );

            QVariantMap payer;
            payer.insert("name", field("name"));            
            payer.insert("email", field("email"));
            if( !field("ytunnus").toString().isEmpty())
                payer.insert("vatnumber", AsiakasToimittajaDlg::yToAlv(field("ytunnus").toString()));
            if( !field("asviite").toString().isEmpty())
                payer.insert("customref", field("asviite").toString());
            if( !field("address").toString().isEmpty())
                payer.insert("address", field("address").toString());
            if( !field("postcode").toString().isEmpty())
                payer.insert("postcode", field("postcode").toString());
            if( !field("town").toString().isEmpty())
                payer.insert("town", field("town").toString());
            if( !field("phone").toString().isEmpty())
                payer.insert("phone", field("phone").toString());
            if( field("verkkolaskulla").toBool()) {
                payer.insert("ovt", field("ovt").toString());
                payer.insert("operator", field("operator").toString());
            }
            tmap.insert("payer", payer);
        }
        QNetworkRequest request( QUrl( kp()->pilvi()->pilviLoginOsoite() + "/subscription" ));
        request.setRawHeader("Authorization", QString("bearer %1").arg( kp()->pilvi()->token() ).toLatin1());
        request.setRawHeader("Content-type", "application/json");
        request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
        QNetworkReply *reply = kp()->networkManager()->post(request, QJsonDocument::fromVariant(tmap).toJson());
        connect( reply, &QNetworkReply::finished, this, &TilausWizard::tilattu);

    }

}

void TilausWizard::tilattu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error() ) {
        qDebug() << reply->readAll();
        QMessageBox::critical(nullptr, tr("Verkkovirhe"), tr("Tilauksen lähettäminen epäonnistui"));
    } else {        
        kp()->pilvi()->paivitaLista();
        if( valintaSivu_->tilaus(PlanModel::PlanRooli).toInt() ) {
            Ui::Kiitos kiitos;
            QDialog dlg(this);
            kiitos.setupUi(&dlg);
            dlg.exec();
        } else {
            QMessageBox::information(this, tr("Kitsas"),tr("Tilauksesi on päivitetty"));
        }
        deleteLater();
    }
}

TilausWizard::TilausYhteysSivu::TilausYhteysSivu() :
    ui( new Ui::TilausYhteys)
{
    ui->setupUi(this);
    setTitle(tr("Laskutustiedot"));

    registerField( "email", ui->emailEdit );
    registerField("name*", ui->nimiEdit);

    registerField("sahkopostilla", ui->emailButton);
    registerField("verkkolaskulla", ui->verkkolaskuButton);
    registerField("ovt", ui->ovtEdit);
    registerField("operator", ui->operaattoriEdit);

    registerField("address", ui->osoiteEdit,"plainText", SIGNAL(textChanged()));
    registerField("postcode", ui->postinumero);
    registerField("town", ui->postitoimipaikka);
    registerField("ytunnus", ui->ytunnusEdit);
    registerField( "asviite", ui->asviite);
    registerField("phone", ui->puhelinEdit);

    ui->ytunnusEdit->setValidator(new YTunnusValidator(false, this));
    connect( ui->postinumero, &QLineEdit::textChanged, this,
             [this] (const QString& numero) { this->ui->postitoimipaikka->setText(Postinumerot::toimipaikka(numero)); });

    connect( ui->ovtEdit, &QLineEdit::textChanged, this, [this] { this->paivitaY(); emit this->completeChanged(); });

    connect( ui->verkkolaskuButton, &QRadioButton::clicked, this, &TilausWizard::TilausYhteysSivu::verkkolaskulle );
    connect( ui->emailButton, &QRadioButton::clicked, this, &TilausWizard::TilausYhteysSivu::verkkolaskulle );

    connect( ui->nimiEdit, &QLineEdit::textChanged, this, &TilausWizard::TilausYhteysSivu::completeChanged);
    connect( ui->emailEdit, &QLineEdit::textChanged, this, &TilausWizard::TilausYhteysSivu::completeChanged);
    connect( ui->operaattoriEdit, &QLineEdit::textChanged, this, &TilausWizard::TilausYhteysSivu::completeChanged);
    connect( ui->ytunnusEdit, &QLineEdit::textChanged, this, &TilausWizard::TilausYhteysSivu::completeChanged);


}

bool TilausWizard::TilausYhteysSivu::isComplete() const
{
    if( ui->nimiEdit->text().isEmpty())
        return false;
    if( ui->emailButton->isChecked()) {
        QRegularExpression emailRe(R"(^.*@.*\.\w+$)");
        return emailRe.match( ui->emailEdit->text()).hasMatch();
    }
    return ui->ovtEdit->text().length() > 11 &&
           ui->operaattoriEdit->text().length() >= 5 &&
            YTunnusValidator::kelpaako(ui->ytunnusEdit->text(), false);
}

void TilausWizard::TilausYhteysSivu::initializePage()
{
    verkkolaskulle();
}

void TilausWizard::TilausYhteysSivu::paivitaY()
{
    QString ovt = ui->ovtEdit->text();
    if( ui->ytunnusEdit->text().isEmpty() && ovt.startsWith("0037") && ovt.length() >= 12 ) {
        QString ytunnus = ovt.mid(4,7) + "-" + ovt.mid(11,1);
        if( YTunnusValidator::kelpaako(ytunnus)) {
            ui->ytunnusEdit->setText(ytunnus);
        }
    }
}

void TilausWizard::TilausYhteysSivu::verkkolaskulle()
{
    bool onko = ui->verkkolaskuButton->isChecked();
    ui->emailLabel->setVisible(!onko);
    ui->emailEdit->setVisible(!onko);

    ui->ovtLabel->setVisible(onko);
    ui->ovtEdit->setVisible(onko);

    ui->operaattoriEdit->setVisible(onko);
    ui->operatorLabel->setVisible(onko);

    emit this->completeChanged();
}

TilausWizard::TilausVahvistusSivu::TilausVahvistusSivu(TilausWizard *velho) :
    ui( new Ui::TilausVahvistus),
    velho_(velho)

{
    setTitle("Vahvista tilaus");
    ui->setupUi(this);
    registerField("ehdot*", ui->ehdotCheck);
}

void TilausWizard::TilausVahvistusSivu::initializePage()
{
    ui->infoLabel->setText( velho_->yhteenveto() );
}
