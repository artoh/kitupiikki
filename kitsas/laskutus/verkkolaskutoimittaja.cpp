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
#include "verkkolaskutoimittaja.h"

#include "db/kirjanpito.h"
#include "rekisteri/asiakastoimittajadlg.h"
#include "pilvi/pilvikysely.h"

#include "myyntilaskuntulostaja.h"

#include <QDebug>
#include <QSettings>
#include <QMessageBox>

VerkkolaskuToimittaja::VerkkolaskuToimittaja(QObject *parent) : QObject(parent)
{

}

void VerkkolaskuToimittaja::lisaaLasku(const QVariantMap &lasku)
{
    laskut_.enqueue(lasku);
}

void VerkkolaskuToimittaja::alustaInit()
{
    init_.insert("ovt", kp()->asetus("OvtTunnus"));
    init_.insert("operaattori", kp()->asetus("Operaattori"));
    init_.insert("nimi", kp()->asetus("Nimi"));
    init_.insert("alvtunnus", AsiakasToimittajaDlg::yToAlv( kp()->asetus("Ytunnus") ) );
    init_.insert("kotipaikka", kp()->asetus("Kotipaikka"));
    init_.insert("osoite", kp()->asetus("Katuosoite"));
    init_.insert("postinumero", kp()->asetus("Postinumero"));
    init_.insert("kaupunki", kp()->asetus("Kaupunki"));

    QVariantList tilit;
    for(QString iban : kp()->asetus("LaskuIbanit").split(',')) {
        QVariantMap tili;
        tili.insert("iban", iban);
        tili.insert("bic", MyyntiLaskunTulostaja::bicIbanilla(iban));
        tilit.append(tili);
    }
    init_.insert("tilit", tilit);
}

bool VerkkolaskuToimittaja::toimitaSeuraava()
{
    if( laskut_.isEmpty())
        return false;

    if( !kp()->pilvi()->kayttajaPilvessa() || !kp()->asetukset()->luku("FinvoiceKaytossa"))
    {
        if( !kp()->asetukset()->luku("FinvoiceKaytossa") )
            QMessageBox::critical(nullptr, tr("Verkkolaskuja ei voi toimittaa"),
                                  tr("Verkkolaskutusta ei ole määritelty käyttöön kirjanpidon asetuksista"));
        else
            QMessageBox::critical(nullptr, tr("Verkkolaskuja ei voi toimittaa"),
                                          tr("Verkkolaskujen toimittaminen edellyttää kirjautumista Kitsaan pilveen"));
        for(int i=0; i < laskut_.count(); i++)
            emit toimitusEpaonnistui();
        return false;
    }


    if( init_.isEmpty())
        alustaInit();

    QVariantMap map = laskut_.dequeue();
    int kumppani = map.value("kumppani").toMap().value("id").toInt();

    KpKysely *kysely = kpk(QString("/kumppanit/%1").arg(kumppani));
    connect( kysely, &KpKysely::vastaus, [this,map] (QVariant* data)
        { this->asiakasSaapuu(data, map);});
    connect( kysely, &KpKysely::virhe, [this] {  this->virhe(tr("Asiakkaan tietojen noutaminen epäonnistui"));} );

    kysely->kysy();

    return true;
}

void VerkkolaskuToimittaja::asiakasSaapuu(const QVariant *data, const QVariantMap &map)
{
    QVariantMap asiakas = data->toMap();

    QVariantMap pyynto;
    pyynto.insert("init", init_);
    pyynto.insert("asiakas", asiakas);
    pyynto.insert("lasku", map.value("lasku") );
    pyynto.insert("rivit", map.value("rivit"));

    QString osoite = kp()->pilvi()->finvoiceOsoite() + "/create";

    PilviKysely *pk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                osoite );
    connect( pk, &PilviKysely::vastaus, [this, map] (QVariant* data){
        this->laskuSaapuu(data, map.value("id").toInt(),map.value("lasku").toMap().value("numero").toInt()); });
    connect( pk, &KpKysely::virhe, [this] {  this->virhe(tr("Verkkolaskun muodostaminen epäonnistui"));} );


    pk->kysy(pyynto);
}

void VerkkolaskuToimittaja::laskuSaapuu(QVariant *data, int tositeId, int laskuId)
{
    QByteArray lasku = data->toByteArray();

    QString hakemisto = kp()->settings()->value( QString("FinvoiceHakemisto/%1").arg(kp()->asetus("UID"))).toString();
    QString tnimi = QString("%1/lasku%2.xml")
            .arg(hakemisto)
            .arg(laskuId,8,10,QChar('0'));
    QFile out(tnimi);
    if( !out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        virhe(tr("Laskutiedoston tallentaminen sijaintiin %1 epäonnistui").arg(hakemisto));
    } else {
        out.write(lasku);
        out.close();
    }

    emit toimitettu(tositeId);
    toimitaSeuraava();
}

void VerkkolaskuToimittaja::virhe(const QString &viesti)
{
    if( !virhe_)
        QMessageBox::critical(nullptr, tr("Verkkolaskun toimittaminen epäonnistui"), viesti);
    emit toimitusEpaonnistui();
    virhe_=true;
}



