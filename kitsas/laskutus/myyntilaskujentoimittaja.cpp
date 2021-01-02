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
#include "model/tositeviennit.h"
#include "model/tositevienti.h"

#include "verkkolaskutoimittaja.h"

#include "db/kirjanpito.h"

#include "smtpclient/SmtpMime"
#include "maaritys/emailmaaritys.h"

#include <QVariantMap>
#include <QList>

#include <QPrintDialog>
#include <QPageLayout>
#include <QPainter>
#include <QPrinter>
#include <QFileDialog>
#include <QSettings>
#include <QProgressDialog>
#include <QMessageBox>



MyyntiLaskujenToimittaja::MyyntiLaskujenToimittaja(QObject *parent) : QObject(parent),
    verkkolaskutoimittaja_(new VerkkolaskuToimittaja(this))
{
    connect( verkkolaskutoimittaja_, &VerkkolaskuToimittaja::toimitettu,
             this, &MyyntiLaskujenToimittaja::merkkaaToimitetuksi);
    connect( verkkolaskutoimittaja_, &VerkkolaskuToimittaja::toimitusEpaonnistui,
             this, &MyyntiLaskujenToimittaja::virhe);
    connect( verkkolaskutoimittaja_, &VerkkolaskuToimittaja::finvoiceToimitettu,
             this, &MyyntiLaskujenToimittaja::toimitettu);
}

bool MyyntiLaskujenToimittaja::toimitaLaskut(const QList<QVariantMap> &laskut)
{
   if( kp()->asetus("LaskuIbanit").isEmpty()) {
       QMessageBox::critical(nullptr, tr("Laskuja ei voi toimittaa"), tr("Laskutustilinumeroita ei ole määritetty"));
       return false;
   }

   laskuja_ = laskut.count();
   dlg_ = new QProgressDialog(tr("Toimitetaan laskuja"),"",0,laskut.count());
   dlg_->setMinimumDuration(500);
   dlg_->setCancelButton(nullptr);

    // Ensin lajitellaan
    for( QVariantMap lasku : laskut)
    {
        int toimitustapa = lasku.value("lasku").toMap().value("laskutapa").toInt();
        if( toimitustapa == LaskuDialogi::PDF)
            tallennettavat_.append(lasku);
        else if(toimitustapa == LaskuDialogi::SAHKOPOSTI)
            sahkopostilla_.append(lasku);
        else if( toimitustapa == LaskuDialogi::VERKKOLASKU)
            verkkolaskutoimittaja_->lisaaLasku(lasku);
        else if(toimitustapa == LaskuDialogi::EITULOSTETA)
            merkkaaToimitetuksi(lasku.value("id").toInt());
        else if( toimitustapa == LaskuDialogi::POSTITUS && kp()->asetukset()->onko("MaventaPostitus")) {
                verkkolaskutoimittaja_->lisaaLasku(lasku);
        }
        else
            tulostettavat_.append(lasku);
    }

    if( !tulostettavat_.isEmpty())
        tulosta();
    if( !tallennettavat_.isEmpty())
        tallenna();
    if( !sahkopostilla_.isEmpty())
        laheta();
        //lahetaSeuraava(Smtp::Unconnected);

    verkkolaskutoimittaja_->toimitaSeuraava();

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
    merkkausMatkalla_ = false;
    toimitetut_++;
    dlg_->setValue(toimitetut_ + virheita_);
    qApp->processEvents();
    merkkaaSeuraava();
}

void MyyntiLaskujenToimittaja::virhe()
{
    virheita_++;
    dlg_->setValue(toimitetut_ + virheita_);
    qApp->processEvents();
    tarkistaValmis();
}

void MyyntiLaskujenToimittaja::tositeSaapuu(QVariant *data)
{
    Tosite tosite(this);
    tosite.lataaData(data);
    tosite.setData(Tosite::TILA, Tosite::LAHETETAAN);

    // Toimitettaessa lasku päivitetään laskun päivämäärä
    // sekä eräpäivä (jos maksuaika jäämässä määriteltyä lyhyemmäksi)
    QVariantMap lasku = tosite.data(Tosite::LASKU).toMap();
    lasku.insert("pvm", kp()->paivamaara());

    QVariantList viennit = tosite.viennit()->vientilLista();
    TositeVienti vienti = viennit.value(0).toMap();
    tosite.asetaLaskupvm( kp()->paivamaara());

    QDate erapvm = tosite.erapvm();
    if( erapvm.isValid() ) {
        if( kp()->paivamaara().daysTo( erapvm ) < kp()->asetukset()->luku("LaskuMaksuaika",0) && kp()->asetukset()->onko("LaskuMaksuaikaVahintaan"))
            erapvm = MyyntiLaskunTulostaja::erapaiva();
        tosite.asetaErapvm(erapvm);
        lasku.insert("erapvm", erapvm);
    }
    viennit[0] = vienti;
    tosite.viennit()->asetaViennit(viennit);

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


void MyyntiLaskujenToimittaja::tarkistaValmis()
{
    if( toimitetut_ + virheita_ == laskuja_ && merkattavat_.isEmpty()) {

        emit kp()->kirjanpitoaMuokattu();
        if( virheita_) {
            if( toimitetut_ > 0)
                emit kp()->onni(tr("%1 laskua toimitettu\n%2 laskun toimittaminen epäonnistui\n"
                                   "Toimittamatta jääneet laskut löytyvät Lähetettävät-välilehdeltä.")
                            .arg(toimitetut_)
                            .arg(virheita_), Kirjanpito::Stop);
            else
                emit kp()->onni(tr("Laskujen toimittaminen epäonnistui tai peruttiin.\n"
                                   "Toimittamatta jääneet laskut löytyvät Lähetettävät-välilehdeltä."), Kirjanpito::Stop);
        } else {
            emit laskutToimitettu();
            if( toimitetut_ > 1)
                emit kp()->onni(tr("%1 laskua toimitettu").arg(toimitetut_), Kirjanpito::Onnistui);
            else
                emit kp()->onni(tr("Lasku toimitettu"), Kirjanpito::Onnistui);
            deleteLater();
        }
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
        for(int i=0; i < tulostettavat_.count(); i++)
        {
            auto tulostettava = tulostettavat_.value(i);
            MyyntiLaskunTulostaja::tulosta(tulostettava, kp()->printer(), &painter, true);
            merkkaaToimitetuksi( tulostettava.value("id").toInt() );
            if( i+1 < tulostettavat_.count())
                kp()->printer()->newPage();
        }
    } else { virhe(); }

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
            } else {
                virhe();
            }
        }
    } else {
        virhe();
    }
    return true;
}

bool MyyntiLaskujenToimittaja::laheta()
{
    bool kpasetus = !kp()->asetukset()->asetus("SmtpServer").isEmpty();
    QString server = kpasetus ? kp()->asetukset()->asetus("SmtpServer") : kp()->settings()->value("SmtpServer").toString();
    int port = kpasetus ? kp()->asetukset()->asetus("SmtpPort").toInt() : kp()->settings()->value("SmtpPort").toInt();
    QString user = kpasetus ? kp()->asetukset()->asetus("SmtpUser") : kp()->settings()->value("SmtpUser").toString();
    QString password = kpasetus ? kp()->asetukset()->asetus("SmtpPassword") : kp()->settings()->value("SmtpPassword").toString();
    int tyyppi = EmailMaaritys::sslIndeksi( kpasetus ? kp()->asetukset()->asetus("EmailSSL") : kp()->settings()->value("EmailSSL").toString() );
    QString kenelta = kpasetus ? kp()->asetukset()->asetus("EmailNimi") : kp()->settings()->value("EmailNimi").toString();
    QString keneltaEmail = kpasetus ? kp()->asetukset()->asetus("EmailOsoite") : kp()->settings()->value("EmailOsoite").toString();
    QString kopioEmail = kpasetus ? kp()->asetukset()->asetus("EmailKopio") : kp()->settings()->value("EmailKopio").toString();


    SmtpClient client(server, port, (SmtpClient::ConnectionType) tyyppi);
    if( !password.isEmpty()) {
        client.setUser(user);
        client.setPassword(password);
    }

    if( !client.connectToHost()) {
        virheita_ += sahkopostilla_.count();
        QMessageBox::critical(dlg_, tr("Sähköpostin lähettäminen epäonnistui"), tr("Sähköpostipalvelimeen %1 yhdistäminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg(server));
        dlg_->setValue(toimitetut_ + virheita_);
        qApp->processEvents();
        tarkistaValmis();
        return false;
    } else if( !password.isEmpty() && !client.login() ) {
        virheita_ += sahkopostilla_.count();
        QMessageBox::critical(dlg_, tr("Sähköpostin lähettäminen epäonnistui"), tr("Sähköpostipalvelimelle %1 kirjautuminen epäonnistui.\nTarkista sähköpostien lähettämisen asetukset.").arg(server));
        dlg_->setValue(toimitetut_ + virheita_);
        qApp->processEvents();
        tarkistaValmis();
        return false;
    }

    bool emailvirheita = false;

    for( QVariant item : sahkopostilla_) {
        QVariantMap data = item.toMap();

        QVariantMap lasku = data.value("lasku").toMap();

        QString kenelleNimi = lasku.value("osoite").toString().split('\n').value(0);
        QString kenelleEmail = lasku.value("email").toString();
        MyyntiLaskunTulostaja tulostaja(data);
        int tyyppi = data.value("tyyppi").toInt();

        QString otsikko = QString("%3 %1 %2").arg(lasku.value("numero").toLongLong()).arg(kp()->asetus("Nimi"))
                .arg(tyyppi == TositeTyyppi::HYVITYSLASKU ? tulostaja.t("hlasku") :
                               (tyyppi == TositeTyyppi::MAKSUMUISTUTUS ? tulostaja.t("maksumuistutus") : tulostaja.t("laskuotsikko")));

        MimeMessage message;
        message.setHeaderEncoding(MimePart::QuotedPrintable);
        message.setSender(new EmailAddress(keneltaEmail, kenelta));
        message.addRecipient(new EmailAddress(kenelleEmail, kenelleNimi));
        if( !kopioEmail.isEmpty())
            message.addBcc(new EmailAddress(kopioEmail));
        message.setSubject(otsikko);

        QString viesti = lasku.value("saate").toString();
        if(viesti.isEmpty())
            viesti = kp()->asetukset()->asetus("EmailSaate");

        if( kp()->asetukset()->luku("EmailMuoto")) {
            if(!viesti.isEmpty())
                viesti.append("\n\n");
            viesti.append( maksutiedot(data) );
        }

        MimeText text(viesti);
        message.addPart(&text);

        QString filename = tulostaja.t("laskuotsikko").toLower() + lasku.value("numero").toString() + ".pdf";
        MimeAttachment attachment(MyyntiLaskunTulostaja::pdf(data), filename);
        message.addPart(&attachment);

        if(client.sendMail(message)) {
            merkkaaToimitetuksi(data.value("id").toInt());
        } else {
            if(!emailvirheita) {
                QMessageBox::critical(dlg_, tr("Sähköpostin lähetys epäonnistui"), tr("Laskujen lähettäminen sähköpostillä epäonnistui."));
                emailvirheita = true;
            }
            virhe();
        }
    }
    return true;
}

void MyyntiLaskujenToimittaja::merkkaaToimitetuksi(int tositeid)
{
    merkattavat_.enqueue(tositeid);
    merkkaaSeuraava();
}

void MyyntiLaskujenToimittaja::merkkaaSeuraava()
{
    if( merkkausMatkalla_)
        return;
    else if( merkattavat_.isEmpty())
        tarkistaValmis();
    else {
        merkkausMatkalla_ = true;
        KpKysely *kysely = kpk(QString("/tositteet/%1").arg(merkattavat_.dequeue()), KpKysely::PATCH);
        QVariantMap map;
        map.insert("tila", Tosite::LAHETETTYLASKU);
        connect( kysely, &KpKysely::vastaus, this, &MyyntiLaskujenToimittaja::toimitettu);
        kysely->kysy(map);
    }
}

QString MyyntiLaskujenToimittaja::maksutiedot(const QVariantMap &data)
{
    MyyntiLaskunTulostaja tulostaja(data);

    double yhteensa = data.value("viennit").toList().value(0).toMap().value("debet").toDouble();
    if( yhteensa < 1e-5)
        return tulostaja.t("eimaksettavaa");   // Ei maksettavaa

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


