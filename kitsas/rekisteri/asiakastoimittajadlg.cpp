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
#include "asiakastoimittajadlg.h"
#include "ui_asiakastoimittajadlg.h"

#include "maamodel.h"
#include "postinumerot.h"

#include "ibandelegaatti.h"
#include "validator/ytunnusvalidator.h"
#include "model/lasku.h"
#include "db/kirjanpito.h"
#include "laskutus/ryhmalasku/kielidelegaatti.h"
#include "maaritys/verkkolasku/verkkolaskumaaritys.h"
#include "pilvi/pilvikysely.h"

#include <QListWidgetItem>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QPushButton>

AsiakasToimittajaDlg::AsiakasToimittajaDlg(QWidget *parent) :
    QDialog (parent),
    ui(new Ui::AsiakasToimittajaDlg)
{
    ui->setupUi(this);
    ui->yEdit->setValidator(new YTunnusValidator(false, this));

    ui->osastoLabel->hide();
    ui->osastoLista->hide();

    connect( ui->postinumeroEdit, &QLineEdit::textChanged, this, &AsiakasToimittajaDlg::haeToimipaikka);
    connect( ui->maaCombo, &QComboBox::currentTextChanged, this, &AsiakasToimittajaDlg::maaMuuttui);
    connect( ui->tilitLista, &QListWidget::itemChanged, this, &AsiakasToimittajaDlg::tarkastaTilit);

    connect( ui->yEdit, &QLineEdit::textEdited, this, &AsiakasToimittajaDlg::haeYTunnarilla);

    connect( ui->nimiEdit, &QLineEdit::textChanged, this, &AsiakasToimittajaDlg::nimiMuuttuu);
    connect( ui->emailEdit, &QLineEdit::textChanged, this, &AsiakasToimittajaDlg::taydennaLaskutavat);
    connect( ui->osoiteEdit, &QPlainTextEdit::textChanged, this, &AsiakasToimittajaDlg::taydennaLaskutavat);
    connect( ui->postinumeroEdit, &QLineEdit::textChanged, this, &AsiakasToimittajaDlg::taydennaLaskutavat);
    connect( ui->kaupunkiEdit, &QLineEdit::textChanged, this, &AsiakasToimittajaDlg::taydennaLaskutavat);
    connect( ui->ovtEdit, &QLineEdit::editingFinished, this, &AsiakasToimittajaDlg::taydennaLaskutavat);
    connect( ui->valittajaEdit, &QLineEdit::editingFinished, this, &AsiakasToimittajaDlg::taydennaLaskutavat);
    connect( ui->osastoLista, &QListWidget::itemClicked, this, &AsiakasToimittajaDlg::osastoValittu);

    connect( ui->haeNappi, &QPushButton::clicked, this, &AsiakasToimittajaDlg::haeNimella);
    connect( ui->buttonBox, &QDialogButtonBox::helpRequested, [] { kp()->ohje("/laskutus/rekisteri"); });

    ui->tilitLista->setItemDelegate( new IbanDelegaatti(this) );
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    alustaKielet();
    taydennaLaskutavat();
}

AsiakasToimittajaDlg::~AsiakasToimittajaDlg()
{
    delete ui;
}

QString AsiakasToimittajaDlg::yToAlv(QString ytunnus)
{
    if( YTunnusValidator::kelpaako(ytunnus))
        return ytunnus.remove('-').insert(0,"FI");
    return QString();
}

QString AsiakasToimittajaDlg::alvToY(QString alvtunnus)
{
    if( !alvtunnus.startsWith("FI"))
        return QString();

    alvtunnus.remove(0,2);
    alvtunnus.insert(7,'-');
    return alvtunnus;
}

void AsiakasToimittajaDlg::muokkaa(int id)
{
    lataa(id);

}

void AsiakasToimittajaDlg::uusi(const QString &nimi)
{
    tauluun();    
    ui->nimiEdit->setText(nimi);    
    ui->maksuaikaSpin->setValue(kp()->asetukset()->luku("LaskuMaksuaika", 14));
    show();
}

void AsiakasToimittajaDlg::ytunnuksella(const QString &ytunnus)
{
    tauluun();
    ui->yEdit->setText(ytunnus);
    haeYTunnarilla();    
}

void AsiakasToimittajaDlg::lataa(int id)
{
    KpKysely* haku = kpk( QString("/kumppanit/%1").arg(id));
    connect( haku, &KpKysely::vastaus,  this, &AsiakasToimittajaDlg::dataSaapuu);
    haku->kysy();
}

void AsiakasToimittajaDlg::tauluun(QVariantMap map)
{
    ladataan_ = true;

    id_ = map.value("id").toInt();

    ui->nimiEdit->setText( map.value("nimi").toString()  );
    ui->maaCombo->setModel( new MaaModel(this));
    QString maa = map.value("maa","fi").toString();

    ui->maaCombo->setCurrentIndex( ui->maaCombo->findData(maa, MaaModel::KoodiRooli));

    if( maa == "fi")
        ui->yEdit->setText( alvToY( map.value("alvtunnus").toString() ));
    else
        ui->alvEdit->setText( map.value("alvtunnus").toString() );
    maaMuuttui();

    ui->emailEdit->setText( map.value("email").toString() );
    ui->osoiteEdit->setPlainText( map.value("osoite").toString() );
    ui->postinumeroEdit->setText( map.value("postinumero").toString());
    ui->kaupunkiEdit->setText( map.value("kaupunki").toString());
    ui->osavaltioEdit->setText(map.value("osavaltio").toString());
    ui->puhelinEdit->setText(map.value("puhelin").toString());

    ui->ovtEdit->setText( map.value("ovt").toString());
    ui->valittajaEdit->setText(map.value("operaattori").toString());

    ui->tilitLista->clear();
    for(const auto& tili : map.value("iban").toList() ) {
        QListWidgetItem* item = new QListWidgetItem(tili.toString());
        item->setFlags( item->flags() | Qt::ItemIsEditable );
        ui->tilitLista->insertItem(0, item);
    }

    ui->ryhmatWidget->valitseRyhmat( map.value("ryhmat").toList() );

    taydennaLaskutavat();

    if( map.contains("kieli"))
        ui->kieliCombo->setCurrentIndex( ui->kieliCombo->findData( map.value("kieli")) );
    if( map.contains("laskutapa"))
        ui->laskutapaCombo->setCurrentIndex( ui->laskutapaCombo->findData( map.value("laskutapa")));
    ui->lisatietoEdit->setPlainText( map.value("lisatiedot").toString());

    if( map.contains("maksuaika")) {
        ui->maksuaikaCheck->setChecked(true);
        ui->maksuaikaSpin->setEnabled(true);
        ui->maksuaikaSpin->setValue(map.value("maksuaika").toInt());
    } else {
        ui->maksuaikaCheck->setChecked(false);
        ui->maksuaikaSpin->setEnabled(false);
        ui->maksuaikaSpin->setValue(kp()->asetukset()->luku("LaskuMaksuaika", 14));
    }

    tarkastaTilit();
    naytaVerkkolasku();
    maventalookup();

    ladataan_ = false;
}

void AsiakasToimittajaDlg::alustaKielet()
{
    KieliDelegaatti::alustaKieliCombo(ui->kieliCombo);
}

bool AsiakasToimittajaDlg::osoiteKunnossa()
{
    const QString maa = ui->maaCombo->currentData(MaaModel::KoodiRooli).toString();
    if( maa == "fi")
        return ui->osoiteEdit->toPlainText().length() > 3 &&
               ui->postinumeroEdit->text().length() == 5 &&
               ui->kaupunkiEdit->text().length() > 1;
    else
        return ui->osoiteEdit->toPlainText().length() > 3 &&
               ui->postinumeroEdit->text().length() > 3 &&
               ui->kaupunkiEdit->text().length() > 1;
}

void AsiakasToimittajaDlg::tuonti(const QVariantMap &map)
{
    QVariantMap uusi;

    if( map.contains("alvtunnus")) {
        tauluun(map);
        accept();
        return;
    }
    else {
        uusi.insert("nimi", map.value("kumppaninimi"));
        uusi.insert("iban", map.value("iban"));
        uusi.insert("osoite", map.value("kumppaniosoite"));
        uusi.insert("postinumero", map.value("kumppanipostinumero"));
        tauluun( uusi );
        haeToimipaikka();
    }
    ui->yEdit->setText( map.value("kumppaniytunnus").toString());
    if( ui->yEdit->hasAcceptableInput())
        haeYTunnarilla();
    else
        show();

}

void AsiakasToimittajaDlg::lisaaRyhmaan(int ryhma)
{
    ui->ryhmatWidget->valitseRyhmat( QVariantList() << ryhma );
}

void AsiakasToimittajaDlg::haeNimella()
{
    QNetworkRequest request( QUrl("http://avoindata.prh.fi/bis/v1/?name="+ui->nimiEdit->text()));
    QNetworkReply *reply = kp()->networkManager()->get(request);
    connect( reply, &QNetworkReply::finished, this, &AsiakasToimittajaDlg::nimellaSaapuu);
}

void AsiakasToimittajaDlg::naytaVerkkolasku()
{
    bool ytunnari = ui->maaCombo->currentData(MaaModel::KoodiRooli).toString() == "fi" &&
            ui->yEdit->hasAcceptableInput();

    ui->yensinOhjeLabel->setVisible(!ytunnari);
    ui->valittajaEdit->setEnabled(ytunnari);
    ui->ovtEdit->setEnabled(ytunnari);

    if( ytunnari && ui->ovtEdit->text().isEmpty() && !ladataan_) {
        QString ovt = "0037" + ui->yEdit->text();
        ovt.remove("-");
        ui->ovtEdit->setText(ovt);
        maventalookup();
    }
}

void AsiakasToimittajaDlg::taydennaLaskutavat()
{
    int laskutapa = ui->laskutapaCombo->currentData().toInt();
    ui->laskutapaCombo->clear();
    ui->laskutapaCombo->addItem(QIcon(":/pic/tulosta.png"), tr("Tulostus"), Lasku::TULOSTETTAVA);
    if( ui->osoiteEdit->toPlainText().length() > 2 && ui->kaupunkiEdit->text().length() > 1)
        ui->laskutapaCombo->addItem(QIcon(":/pic/kirje.png"),tr("Postitus"), Lasku::POSTITUS);
    QRegularExpression emailRe(R"(^.*@.*\.\w+$)");
    const QString& spostiosoite = ui->emailEdit->text();
    if( emailRe.match( spostiosoite ).hasMatch() )
        ui->laskutapaCombo->addItem(QIcon(":/pic/email.png"), tr("Sähköposti"), Lasku::SAHKOPOSTI);
    if( ui->ovtEdit->text().length() > 11 && ui->valittajaEdit->text().length() > 5 ) {
        if( osoiteKunnossa()) {
            ui->laskutapaCombo->addItem(QIcon(":/pic/verkkolasku.png"), tr("Verkkolasku"), Lasku::VERKKOLASKU);
            if( kp()->asetukset()->onko("FinvoiceSuosi"))
                laskutapa = Lasku::VERKKOLASKU;
            ui->osoiteUrputus->hide();
        } else {
            ui->osoiteUrputus->show();
        }
    } else {
        ui->osoiteUrputus->show();
    }

    int indeksi = ui->laskutapaCombo->findData(laskutapa);
    if( indeksi > 0)
        ui->laskutapaCombo->setCurrentIndex(indeksi);

}

void AsiakasToimittajaDlg::maventalookup()
{
    const QString nimi = ui->nimiEdit->text().trimmed();

    if(kp()->asetukset()->luku("FinvoiceKaytossa") == VerkkolaskuMaaritys::MAVENTA && kp()->pilvi()->kayttajaPilvessa() ) {

        QString osoite = kp()->pilvi()->finvoiceOsoite() + "lookup";

        PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::GET,
                    osoite );
        pk->lisaaAttribuutti("mybid", kp()->asetukset()->ytunnus());
        if( ui->maaCombo->currentData(MaaModel::KoodiRooli).toString() == "fi" &&
                ui->yEdit->hasAcceptableInput())
            pk->lisaaAttribuutti("bid", ui->yEdit->text());
        else if( nimi.length() > 3)
            pk->lisaaAttribuutti("name", ui->nimiEdit->text());
        else
            return; // Ei edellytyksiä nimellä hakemiseen
        connect( pk, &PilviKysely::vastaus, this, &AsiakasToimittajaDlg::maventalookupSaapuu);
        pk->kysy();
    }
}

void AsiakasToimittajaDlg::maventalookupSaapuu(QVariant* data) {
    QVariantList list = data->toList();

    ui->osastoLista->clear();

    for(const QVariant& var : qAsConst( list )) {
        QVariantMap map=var.toMap();

        const QString& ovt = map.value("eia").toString();
        const QString& valittaja = map.value("operator").toString();

        const QVariantMap participant = map.value("participant").toMap();
        QString bid = participant.value("bid").toString();
        const QString& nimi = participant.value("name").toString();


        if(ovt.contains(QRegularExpression("\\D"))) {
            continue;
        }

        if( ui->valittajaEdit->text().isEmpty()) {
            ui->ovtEdit->setText(ovt);
            ui->valittajaEdit->setText(valittaja);
        }

        if( ui->yEdit->text().isEmpty()) {
            if( bid.startsWith("FI"))
                bid = alvToY(bid);
            if( YTunnusValidator::kelpaako(bid))
                ui->yEdit->setText(bid);
        }
        if( ui->nimiEdit->text().isEmpty()) {
            ui->nimiEdit->setText(nimi);
        }

        QListWidgetItem *item = new QListWidgetItem(nimi, ui->osastoLista);
        item->setData( OVTTUNNUS, ovt );
        item->setData( VALITTAJA, valittaja);

        const QString nykyinenOvt = ui->ovtEdit->text();
        const QString nykyinenValittaja = ui->valittajaEdit->text();

        if( !nykyinenOvt.isEmpty() && nykyinenOvt == ovt &&
            !nykyinenValittaja.isEmpty() && nykyinenValittaja == valittaja) {
            ui->osastoLista->setCurrentItem(item);
        }

    }

    naytaVerkkolasku();
    taydennaLaskutavat();

    ui->osastoLabel->setVisible( ui->osastoLista->count() );
    ui->osastoLista->setVisible( ui->osastoLista->count() );
}

void AsiakasToimittajaDlg::osastoValittu()
{
    QListWidgetItem* item = ui->osastoLista->currentItem();
    if( item ) {
        ui->ovtEdit->setText( item->data(OVTTUNNUS).toString() );
        ui->valittajaEdit->setText( item->data(VALITTAJA).toString());
        naytaVerkkolasku();
        taydennaLaskutavat();
    }
}

void AsiakasToimittajaDlg::dataTauluun(const QVariant &data)
{
    QVariantMap tieto = data.toMap().value("results").toList().value(0).toMap();
    ui->nimiEdit->setText( tieto.value("name").toString() );
    ui->yEdit->setText(tieto.value("businessId").toString());
    QVariantList osoitteet = tieto.value("addresses").toList();
    for(const auto& item : qAsConst( osoitteet )) {
        QVariantMap osoite = item.toMap();
        if( osoite.value("endDate").toDate().isValid() )
            continue;
        ui->osoiteEdit->setPlainText( osoite.value("street").toString() );
        ui->postinumeroEdit->setText( osoite.value("postCode").toString() );
        ui->kaupunkiEdit->setText( osoite.value("city").toString());
        break;
    }
}

void AsiakasToimittajaDlg::tarkastaTilit()
{
    bool tyhjat = false;
    for(int i=0; i < ui->tilitLista->count(); i++)
        if( ui->tilitLista->item(i)->data(Qt::EditRole).toString().isEmpty())
            tyhjat = true;

    if( !tyhjat) {
        QListWidgetItem* item = new QListWidgetItem("", ui->tilitLista);
        item->setFlags( item->flags() | Qt::ItemIsEditable );
    }
}

void AsiakasToimittajaDlg::maaMuuttui()
{
    QString maa = ui->maaCombo->currentData(MaaModel::KoodiRooli).toString();   
    QString alvReg = ui->maaCombo->currentData(MaaModel::AlvRegExpRooli).toString();

    ui->yLabel->setVisible( maa == "fi");
    ui->yEdit->setVisible( maa == "fi");    

    ui->alvlabel->setVisible( maa != "fi" && !alvReg.isEmpty());
    ui->alvEdit->setVisible( maa != "fi" && !alvReg.isEmpty());

    ui->osavaltioLabel->setVisible( maa != "fi");
    ui->osavaltioEdit->setVisible( maa != "fi");

    if(ui->alvEdit->isVisible() && ui->alvEdit->text().length() < 3)
        ui->alvEdit->setText( alvReg.left(2) );
    ui->alvEdit->setValidator( new QRegularExpressionValidator(QRegularExpression( alvReg ), this) );
}

void AsiakasToimittajaDlg::haeToimipaikka()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty() && ui->maaCombo->currentData(MaaModel::KoodiRooli).toString() == "fi")
        ui->kaupunkiEdit->setText(toimipaikka);
}

void AsiakasToimittajaDlg::nimiMuuttuu()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled( !ui->nimiEdit->text().isEmpty() );
}

void AsiakasToimittajaDlg::accept()
{
    QVariantMap map;

    map.insert("nimi", ui->nimiEdit->text());

    QString maa =  ui->maaCombo->currentData(MaaModel::KoodiRooli).toString();
    map.insert("maa", maa);

    if( maa == "fi") {
        if( !ui->yEdit->text().isEmpty())
            map.insert("alvtunnus", yToAlv( ui->yEdit->text() ));
    } else {
        if( ui->alvEdit->isVisible() && ui->alvEdit->text().length()>3 && ui->alvEdit->hasAcceptableInput())
            map.insert("alvtunnus", ui->alvEdit->text());
    }

    if( !ui->osoiteEdit->toPlainText().isEmpty())
        map.insert("osoite", ui->osoiteEdit->toPlainText());

    if( !ui->postinumeroEdit->text().isEmpty() )
        map.insert("postinumero", ui->postinumeroEdit->text());

    if( !ui->kaupunkiEdit->text().isEmpty())
        map.insert("kaupunki", ui->kaupunkiEdit->text());
    if( !ui->osavaltioEdit->text().isEmpty() && ui->osavaltioEdit->isVisible())
        map.insert("osavaltio", ui->osavaltioEdit->text());

    if( !ui->emailEdit->text().isEmpty())
        map.insert("email", ui->emailEdit->text());
    if( !ui->puhelinEdit->text().isEmpty())
        map.insert("puhelin", ui->puhelinEdit->text());

    QVariantList tililista;
    for(int i=0; i<ui->tilitLista->count(); i++)
        if( !ui->tilitLista->item(i)->data(Qt::EditRole).toString().isEmpty())
            tililista.append( ui->tilitLista->item(i)->data(Qt::EditRole) );

    map.insert("iban", tililista);
    map.insert("kieli", ui->kieliCombo->currentData().toString());
    map.insert("laskutapa", ui->laskutapaCombo->currentData().toInt());
    map.insert("lisatiedot", ui->lisatietoEdit->toPlainText());

    if( !ui->ovtEdit->text().isEmpty() )
        map.insert("ovt", ui->ovtEdit->text());
    if( !ui->valittajaEdit->text().isEmpty())
        map.insert("operaattori", ui->valittajaEdit->text());

    if( ui->maksuaikaCheck->isChecked())
        map.insert("maksuaika", ui->maksuaikaSpin->value());

    QVariantList valitutRyhmat = ui->ryhmatWidget->valitutRyhmat();
    if( !valitutRyhmat.isEmpty())
        map.insert("ryhmat", valitutRyhmat);

    KpKysely* kysely = nullptr;    
        kysely = id_ ? kpk( QString("/kumppanit/%1").arg(id_) , KpKysely::PUT ) :
                       kpk( "/kumppanit", KpKysely::POST);

    connect(kysely, &KpKysely::vastaus, this, &AsiakasToimittajaDlg::tallennusValmis  );
    kp()->odotusKursori(true);
    kysely->kysy(map);

}

void AsiakasToimittajaDlg::reject()
{
    QDialog::reject();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
}

void AsiakasToimittajaDlg::dataSaapuu(QVariant *data)
{     
    tauluun( data->toMap() );
    show();
}


void AsiakasToimittajaDlg::haeYTunnarilla()
{
    if( ui->yEdit->hasAcceptableInput() && !ladataan_) {
        QNetworkRequest request( QUrl("http://avoindata.prh.fi/bis/v1/" + ui->yEdit->text()));
        QNetworkReply *reply = kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &AsiakasToimittajaDlg::yTietoSaapuu);
    }
}

void AsiakasToimittajaDlg::yTietoSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    QVariant var = QJsonDocument::fromJson( reply->readAll() ).toVariant();
    if( var.toMap().value("results").toList().isEmpty()) {
        if( !isVisible())
            show();
        naytaVerkkolasku();
        return;
    }
    dataTauluun(var);
    naytaVerkkolasku();

    if( !isVisible() )
        accept();

}

void AsiakasToimittajaDlg::nimellaSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    QVariant var = QJsonDocument::fromJson( reply->readAll() ).toVariant();

    if( var.toMap().value("results").toList().length() == 1) {
        dataTauluun(var);
    }
    maventalookup();
}

void AsiakasToimittajaDlg::tallennusValmis(QVariant *data)
{
    kp()->odotusKursori(false);
    QVariantMap map = data->toMap();

    QDialog::accept();

    emit kumppaniTallennettu(map);
    emit kp()->kirjanpitoaMuokattu();
}
