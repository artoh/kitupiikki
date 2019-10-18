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

#include "db/kirjanpito.h"

#include <QListWidgetItem>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>

AsiakasToimittajaDlg::AsiakasToimittajaDlg(QWidget *parent) :
    QDialog (parent),
    ui(new Ui::AsiakasToimittajaDlg)
{
    ui->setupUi(this);
    ui->yEdit->setValidator(new YTunnusValidator(false));

    connect( ui->postinumeroEdit, &QLineEdit::textChanged, this, &AsiakasToimittajaDlg::haeToimipaikka);
    connect( ui->maaCombo, &QComboBox::currentTextChanged, this, &AsiakasToimittajaDlg::maaMuuttui);
    connect( ui->tilitLista, &QListWidget::itemChanged, this, &AsiakasToimittajaDlg::tarkastaTilit);

    connect( ui->yEdit, &QLineEdit::textEdited, this, &AsiakasToimittajaDlg::haeYTunnarilla);
    connect( ui->yEdit, &QLineEdit::editingFinished, this, &AsiakasToimittajaDlg::haeYTunnarilla);

    ui->tilitLista->setItemDelegate( new IbanDelegaatti() );
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

    exec();
}

void AsiakasToimittajaDlg::ytunnuksella(const QString &ytunnus)
{
    tauluun();
    ui->yEdit->setText(ytunnus);
    haeYTunnarilla();
    exec();
}

void AsiakasToimittajaDlg::lataa(int id)
{
    KpKysely* haku = kpk( QString("/kumppanit/%1").arg(id));
    connect( haku, &KpKysely::vastaus,  this, &AsiakasToimittajaDlg::dataSaapuu);
    haku->kysy();
}

void AsiakasToimittajaDlg::tauluun(QVariantMap map)
{
    id_ = map.value("id").toInt();

    ui->nimiEdit->setText( map.value("nimi").toString()  );
    ui->maaCombo->setModel( new MaaModel(this));
    QString maa = map.value("maa","fi").toString();

    ui->maaCombo->setCurrentIndex( ui->maaCombo->findData(maa, MaaModel::KoodiRooli));

    if( maa == "fi")
        ui->yEdit->setText( alvToY( map.value("alvtunnus").toString() ));
    else
        ui->alvEdit->setText( map.value("alvtunnus").toString() );

    ui->emailEdit->setText( map.value("email").toString() );
    ui->osoiteEdit->setPlainText( map.value("osoite").toString() );
    ui->postinumeroEdit->setText( map.value("postinumero").toString());
    ui->kaupunkiEdit->setText( map.value("kaupunki").toString());

    ui->ovtEdit->setText( map.value("ovt").toString());
    ui->valittajaEdit->setText(map.value("operaattori").toString());

    ui->tilitLista->clear();
    for(auto tili : map.value("iban").toList() ) {
        QListWidgetItem* item = new QListWidgetItem(tili.toString(), ui->tilitLista);
        item->setFlags( item->flags() | Qt::ItemIsEditable );
    }

    maaMuuttui();
    tarkastaTilit();
}

void AsiakasToimittajaDlg::tuonti(const QVariantMap &map)
{
    QVariantMap uusi;
    uusi.insert("nimi", map.value("kumppaninimi"));
    uusi.insert("iban", map.value("iban"));
    tauluun( uusi );
    ui->yEdit->setText( map.value("kumppaniytunnus").toString());
    haeYTunnarilla();
    exec();
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
    ui->yLabel->setVisible( maa == "fi");
    ui->yEdit->setVisible( maa == "fi");
    ui->alvlabel->setVisible( maa != "fi");
    ui->alvEdit->setVisible( maa != "fi");
    ui->alvEdit->setValidator( new QRegularExpressionValidator(QRegularExpression( ui->maaCombo->currentData(MaaModel::AlvRegExpRooli).toString() ), this) );
}

void AsiakasToimittajaDlg::haeToimipaikka()
{
    QString toimipaikka = Postinumerot::toimipaikka( ui->postinumeroEdit->text() );
    if( !toimipaikka.isEmpty() && ui->maaCombo->currentData(MaaModel::KoodiRooli).toString() == "fi")
        ui->kaupunkiEdit->setText(toimipaikka);
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
        if( !ui->alvEdit->text().isEmpty())
            map.insert("alvtunnus", ui->alvEdit->text());
    }

    if( !ui->osoiteEdit->toPlainText().isEmpty())
        map.insert("osoite", ui->osoiteEdit->toPlainText());

    if( !ui->postinumeroEdit->text().isEmpty() )
        map.insert("postinumero", ui->postinumeroEdit->text());

    if( !ui->kaupunkiEdit->text().isEmpty())
        map.insert("kaupunki", ui->kaupunkiEdit->text());

    if( !ui->emailEdit->text().isEmpty())
        map.insert("email", ui->emailEdit->text());

    QVariantList tililista;
    for(int i=0; i<ui->tilitLista->count(); i++)
        if( !ui->tilitLista->item(i)->data(Qt::EditRole).toString().isEmpty())
            tililista.append( ui->tilitLista->item(i)->data(Qt::EditRole) );

    map.insert("iban", tililista);

    if( !ui->ovtEdit->text().isEmpty() )
        map.insert("ovt", ui->ovtEdit->text());
    if( !ui->valittajaEdit->text().isEmpty())
        map.insert("operaattori", ui->valittajaEdit->text());

    KpKysely* kysely = nullptr;    
        kysely = id_ ? kpk( QString("/kumppanit/%1").arg(id_) , KpKysely::PUT ) :
                       kpk( "/kumppanit", KpKysely::POST);

    qDebug() << map;

    connect(kysely, &KpKysely::vastaus, this, &AsiakasToimittajaDlg::tallennusValmis  );
    kysely->kysy(map);

}

void AsiakasToimittajaDlg::dataSaapuu(QVariant *data)
{     
    tauluun( data->toMap() );
    exec();
}


void AsiakasToimittajaDlg::haeYTunnarilla()
{
    if( ui->yEdit->hasAcceptableInput() ) {
        QNetworkRequest request( QUrl("http://avoindata.prh.fi/bis/v1/" + ui->yEdit->text()));
        QNetworkReply *reply = kp()->networkManager()->get(request);
        connect( reply, &QNetworkReply::finished, this, &AsiakasToimittajaDlg::yTietoSaapuu);
    }
}

void AsiakasToimittajaDlg::yTietoSaapuu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    QVariant var = QJsonDocument::fromJson( reply->readAll() ).toVariant();
    if( var.toMap().value("results").toList().isEmpty())
        return;

    QVariantMap tieto = var.toMap().value("results").toList().first().toMap();


    ui->nimiEdit->setText( tieto.value("name").toString() );
    QVariantMap osoite = tieto.value("addresses").toList().first().toMap();
    ui->osoiteEdit->setPlainText( osoite.value("street").toString() );
    ui->postinumeroEdit->setText( osoite.value("postCode").toString() );
    ui->kaupunkiEdit->setText( osoite.value("city").toString());


}

void AsiakasToimittajaDlg::tallennusValmis(QVariant *data)
{
    QVariantMap map = data->toMap();
    int id = map.value("id").toInt();
    QString nimi = map.value("nimi").toString();

    QDialog::accept();

    qDebug() << "Tallennettu " << id << " - " << nimi;

    emit tallennettu(id, nimi);
}
