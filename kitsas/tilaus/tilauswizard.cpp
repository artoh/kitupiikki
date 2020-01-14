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

#include "ui_tilausyhteys.h"
#include "ui_tilausvahvistus.h"

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
    KpKysely *kysely = kpk( kp()->pilvi()->pilviLoginOsoite() + "/subscription" );
    connect( kysely, &KpKysely::vastaus, this, &TilausWizard::dataSaapuu );
    kysely->kysy();
}

QString TilausWizard::yhteenveto()
{
    QString txt = tr("Tilaamasi paketti: %1 \n").arg(valintaSivu_->tilaus(PlanModel::NIMI).toString());
    if( !valintaSivu_->tilaus(PlanModel::PlanRooli).toInt())
        txt.append( tr("Maksuton käyttäjä voi lisätä enintään 50 tositetta, eikä pääse "
                       "hyödyntämään lisätoimintoja kuten skannattujen kuittien tekstintunnistusta."));
    else {
        txt.append( tr("Voit tallentaa pilveen enintään %1 kirjanpitoa.")
                    .arg( valintaSivu_->tilaus(PlanModel::PilviaRooli ).toInt() ) );

        double hinta = valintaSivu_->tilaus(PlanModel::HintaRooli).toDouble();

        txt.append( tr("\n\nHinta %L1 € ").arg( hinta ,0,'f',2 ));
        if( field("kuukausittain").toBool())
            txt.append( tr("laskutettuna kuukausittain\n") );
        else
            txt.append( tr("laskutettuna vuosittain\n"));

        double hyvitys = current_.value("refund").toDouble();
        if( hyvitys > hinta)
            hyvitys = hinta;

        if( hyvitys > 1) {
            txt.append(tr("Ensimmäiseltä laskulta vähennetään nykyistä tilaustasi %L1€, "
                       "jolloin maksettavaa jää L1€.")
                    .arg(hyvitys,0,'f',2)
                    .arg(hinta - hyvitys, 0, 'f', 2) );
        }
        txt.append(tr("\nLasku toimitetaan sähköpostilla osoitteeseen %1")
                   .arg( field("email").toString() ));

        txt.append(tr("\n\nLaskun saajaksi merkitään\n"));
        txt.append( field("name").toString() + "\n");
        txt.append( field("address").toString());

        if( !field("asviite").toString().isEmpty())
            txt.append("\n\nAsiakkaan viite: " + field("asviite").toString());
    }

    return txt;

}

void TilausWizard::dataSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
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

    bool kuukausittain = current_.value("period").toInt() == 1;
    setField("name", current_.value("name").toString());
    setField("address", current_.value("address"));
    setField("asviite", current_.value("customerref"));

    planModel_->alusta( map.value("plans").toList(), kuukausittain);
    valintaSivu_->alusta(nykyplan, kuukausittain,
                         current_.value("refund").toDouble());

    if( exec() )
    {
        KpKysely *tallennus = kpk( kp()->pilvi()->pilviLoginOsoite() + "/subscription", KpKysely::POST );
        QVariantMap tmap;
        int plan = valintaSivu_->tilaus(PlanModel::PlanRooli).toInt();
        tmap.insert("plan", plan);

        if( plan) {
            tmap.insert("period", field("kuukausittain").toBool() ? 1 : 12 );
            tmap.insert("email", field("email"));
            tmap.insert("name", field("name"));
            tmap.insert("address", field("address"));
            tmap.insert("customref", field("asviite"));
        }
        connect( tallennus, &KpKysely::vastaus, kp()->pilvi(), &PilviModel::paivitaLista);
        tallennus->kysy(tmap);
    }
    deleteLater();
}

TilausWizard::TilausYhteysSivu::TilausYhteysSivu() :
    ui( new Ui::TilausYhteys)
{
    ui->setupUi(this);
    setTitle(tr("Laskutustiedot"));

    registerField( "email*", ui->emailEdit );
    registerField("name*", ui->nimiEdit);

    registerField("address", ui->osoiteEdit,"plainText", SIGNAL(textChanged()));
    registerField( "asviite", ui->asviite);

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
