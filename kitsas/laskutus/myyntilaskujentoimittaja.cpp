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
#include "myyntilaskujentoimittaja.h"
#include "laskudialogi.h"
#include "myyntilaskuntulostaja.h"
#include "model/tosite.h"

#include "db/kirjanpito.h"

#include <QVariantMap>
#include <QList>

#include <QPrintDialog>
#include <QPageLayout>
#include <QPainter>
#include <QPrinter>
#include <QFileDialog>
#include <QSettings>

#include "smtp.h"

bool MyyntiLaskujenToimittaja::toimitaLaskut(const QList<QVariantMap> &laskut)
{
   laskuja_ = laskut.count();

    // Ensin lajitellaan
    for( QVariantMap lasku : laskut)
    {
        int toimitustapa = lasku.value("lasku").toMap().value("laskutapa").toInt();
        if( toimitustapa == LaskuDialogi::TULOSTETTAVA)
            tulostettavat_.append(lasku);
        else if( toimitustapa == LaskuDialogi::PDF)
            tallennettavat_.append(lasku);
        else if(toimitustapa == LaskuDialogi::SAHKOPOSTI)
            sahkopostilla_.append(lasku);
        else if(toimitustapa == LaskuDialogi::EITULOSTETA)
            merkkaaToimitetuksi(lasku.value("id").toInt());
    }

    if( !tulostettavat_.isEmpty())
        tulosta();
    if( !tallennettavat_.isEmpty())
        tallenna();
    if( !sahkopostilla_.isEmpty())
        lahetaSeuraava(Smtp::Unconnected);

    return true;

}

void MyyntiLaskujenToimittaja::toimitaLaskut(const QList<int> &tositteet)
{
    if( !tositteet.isEmpty()) {
        tilattavat_ = tositteet;
        tilaaSeuraavaLasku();
    }
}

void MyyntiLaskujenToimittaja::toimitettu()
{    
    toimitetut_++;
    if( toimitetut_ == laskuja_) {
        emit laskutToimitettu();
        emit kp()->kirjanpitoaMuokattu();
        deleteLater();
    }
}

void MyyntiLaskujenToimittaja::tositeSaapuu(QVariant *data)
{
    Tosite tosite(this);
    tosite.lataaData(data);
    tosite.setData(Tosite::TILA, Tosite::LAHETETAAN);

    QVariantMap lasku = tosite.data(Tosite::LASKU).toMap();
    lasku.insert("pvm", kp()->paivamaara());
    tosite.setData(Tosite::LASKU, lasku);

    KpKysely *tallennuskysely = kpk(QString("/tositteet/%1").arg(tosite.id()), KpKysely::PUT);
    connect( tallennuskysely, &KpKysely::vastaus, this, &MyyntiLaskujenToimittaja::tositeTallennettu);
    tallennuskysely->kysy(tosite.tallennettava());

}

void MyyntiLaskujenToimittaja::tositeTallennettu(QVariant *data)
{
    QVariantMap map = data->toMap();
    toimitettavat_.append(map);

    // Päivitetään liite
    QByteArray liite = MyyntiLaskunTulostaja::pdf( map );
    KpKysely *liitetallennus = kpk( QString("/liitteet/%1/lasku").arg(map.value("id").toInt()), KpKysely::PUT);
    QMap<QString,QString> meta;
    meta.insert("Filename", QString("lasku%1.pdf").arg( map.value("lasku").toMap().value("numero").toInt() ) );
    liitetallennus->lahetaTiedosto(liite, meta);

    if( tilattavat_.isEmpty())
        toimitaLaskut( toimitettavat_ );
    else
        tilaaSeuraavaLasku();
}

void MyyntiLaskujenToimittaja::lahetaSeuraava(int status)
{
    qDebug() << " **LS ** " << status;

    if( status == Smtp::Failed) {
        if( !emailvirheita_)
            QMessageBox::critical(nullptr, tr("Sähköpostin lähetys epäonnistui"), tr("Laskujen lähettäminen sähköpostillä epäonnistui. Tarkista sähköpostiasetukset."));
        emailvirheita_ = true;
    } else if( status == Smtp::Connecting || status == Smtp::Sending) {
        return;
    } else if( status == Smtp::Send) {
        QVariantMap lahetetty = sahkopostilla_.value(0);
        qDebug() << "Send " << lahetetty.value("id").toInt();
        merkkaaToimitetuksi( lahetetty.value("id").toInt());
        sahkopostilla_.removeFirst();
    }

    if(!sahkopostilla_.isEmpty()) {
        Smtp *smtp = kp()->asetukset()->asetus("SmtpServer").isEmpty() ?
                    new Smtp( kp()->settings()->value("SmtpUser").toString(),
                              kp()->settings()->value("SmtpPassword").toString(),
                              kp()->settings()->value("SmtpServer").toString(),
                              kp()->settings()->value("SmtpPort").toInt() ) :
                    new Smtp( kp()->asetus("SmtpUser"),
                              kp()->asetus("SmtpPassword"),
                              kp()->asetus("SmtpServer"),
                              kp()->asetukset()->luku("SmtpPort"));

        QVariantMap data = sahkopostilla_.value(0);
        QVariantMap lasku = data.value("lasku").toMap();

        QString keneltaNimi = kp()->asetukset()->asetus("SmtpServer").isEmpty() ?
                    kp()->settings()->value("EmailNimi").toString() :
                    kp()->asetus("EmailNimi");
        QString keneltaEmail = kp()->asetukset()->asetus("SmtpServer").isEmpty() ?
                    kp()->settings()->value("EmailOsoite").toString() :
                    kp()->asetus("EmailOsoite");
        QString kenelleNimi = lasku.value("osoite").toString().split('\n').value(0);
        QString kenelleEmail = lasku.value("email").toString();

        QString kenelta = QString("=?utf-8?b?%1?= <%2>").arg( QString(keneltaNimi.toUtf8().toBase64())  )
                                                    .arg(keneltaEmail);
        QString kenelle = QString("=?utf-8?b?%1?= <%2>").arg( QString( kenelleNimi.toUtf8().toBase64()) )
                                                .arg(kenelleEmail );

        qDebug() << kenelle;

        // Viestirungon hakeminen ja virtuaaliviivakoodin laskeminen ;)
        QString viesti = lasku.value("saate").toString();
        if( kp()->asetukset()->luku("EmailMuoto")) {
            if(!viesti.isEmpty())
                viesti.append("\n\n");
            viesti.append( maksutiedot(data) );
        }
        MyyntiLaskunTulostaja tulostaja(data);
        int tyyppi = data.value("tyyppi").toInt();

        QString otsikko = QString("%3 %1 %2").arg(lasku.value("numero").toLongLong()).arg(kp()->asetus("Nimi"))
                .arg(tyyppi == TositeTyyppi::HYVITYSLASKU ? tulostaja.t("hlasku") :
                               (tyyppi == TositeTyyppi::MAKSUMUISTUTUS ? tulostaja.t("maksumuistutus") : tulostaja.t("laskuotsikko")));

        connect( smtp, &Smtp::status, this, &MyyntiLaskujenToimittaja::lahetaSeuraava);
        smtp->lahetaLiitteella(kenelta, kenelle, otsikko, viesti, "lasku.pdf", MyyntiLaskunTulostaja::pdf(data));
    }
}

bool MyyntiLaskujenToimittaja::tulosta()
{
    QPageLayout vanhaleiska = kp()->printer()->pageLayout();
    QPageLayout uusileiska = vanhaleiska;
    uusileiska.setUnits(QPageLayout::Millimeter);
    uusileiska.setMargins( QMarginsF(5.0,5.0,5.0,5.0));
    kp()->printer()->setPageLayout(uusileiska);



    QPrintDialog printDialog( kp()->printer() );
    if( printDialog.exec())
    {
        QPainter painter( kp()->printer() );
        for(QVariantMap tulostettava : tulostettavat_)
        {
            MyyntiLaskunTulostaja::tulosta(tulostettava, kp()->printer(), &painter, true);
            merkkaaToimitetuksi( tulostettava.value("id").toInt() );
        }
    }

    kp()->printer()->setPageLayout(vanhaleiska);
    return true;
}

bool MyyntiLaskujenToimittaja::tallenna()
{
    QString hakemistopolku = QFileDialog::getExistingDirectory(nullptr, tr("Tallennettavien laskujen kansio"));
    if( !hakemistopolku.isEmpty()) {
        QDir hakemisto( hakemistopolku );
        for( QVariantMap tallennettava : tallennettavat_) {
            int laskunumero = tallennettava.value("lasku").toMap().value("numero").toInt();
            QFile ulos( hakemisto.filePath( QString("lasku%1.pdf").arg(laskunumero,8,10,QChar('0'))) );
            if( ulos.open(QIODevice::WriteOnly) ) {
                ulos.write( MyyntiLaskunTulostaja::pdf(tallennettava) );
                merkkaaToimitetuksi( tallennettava.value("id").toInt() );
            }
        }
    }
    return true;
}

void MyyntiLaskujenToimittaja::merkkaaToimitetuksi(int tositeId)
{
    KpKysely *kysely = kpk(QString("/tositteet/%1").arg(tositeId), KpKysely::PATCH);
    QVariantMap map;
    map.insert("tila", Tosite::LAHETETTYLASKU);
    connect( kysely, &KpKysely::vastaus, this, &MyyntiLaskujenToimittaja::toimitettu);
    kysely->kysy(map);
    qDebug() << "toimitettu " << tositeId;
}

QString MyyntiLaskujenToimittaja::maksutiedot(const QVariantMap &data)
{
    MyyntiLaskunTulostaja tulostaja(data);

    double yhteensa = data.value("viennit").toList().value(0).toMap().value("debet").toDouble();
    if( yhteensa < 1e-5)
        return tulostaja.t("Ei maksettavaa");   // Ei maksettavaa

    QVariantMap lasku = data.value("lasku").toMap();

    QString txt = tulostaja.t("erapvm") + " " + lasku.value("erapvm").toDate().toString("dd.MM.yyyy") + "\n";
    txt.append(tulostaja.t("Yhteensa") + " " + QString::number(yhteensa,'f',2) + " €\n");
    txt.append(tulostaja.t("viitenro") + " " + tulostaja.muotoiltuViite() + "\n");
    txt.append(tulostaja.t("iban") + " " + tulostaja.iban() + "\n");
    txt.append(tulostaja.t("virtviiv") + " " + tulostaja.virtuaaliviivakoodi());

    return txt;
}

void MyyntiLaskujenToimittaja::tilaaSeuraavaLasku()
{
    int id = tilattavat_.takeFirst();
    KpKysely* kysely = kpk(QString("/tositteet/%1").arg(id));
    connect( kysely, &KpKysely::vastaus, this, &MyyntiLaskujenToimittaja::tositeSaapuu );
    kysely->kysy();
}

MyyntiLaskujenToimittaja::MyyntiLaskujenToimittaja(QObject *parent) : QObject(parent)
{

}
