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
#include "arkistoija.h"

#include "db/kirjanpito.h"
#include "arkistohakemistodialogi.h"

#include "raportti/raportoija.h"
#include "raportti/paakirja.h"
#include "raportti/paivakirja.h"
#include "raportti/taseerittelija.h"
#include "raportti/tilikarttalistaaja.h"
#include "raportti/tositeluettelo.h"

#include "sqlite/sqlitemodel.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QFileDialog>

#include <QDebug>
#include <QApplication>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QSettings>
#include <QProgressDialog>


Arkistoija::Arkistoija(const Tilikausi &tilikausi, QObject *parent)
    : QObject(parent), tilikausi_(tilikausi)
{

}

void Arkistoija::arkistoi()
{
    if( luoHakemistot() ) {
        arkistoiTositteet();
        arkistoiRaportit();
    }
}

bool Arkistoija::luoHakemistot()
{

    QString arkistopolku = kp()->settings()->value("arkistopolku/" + kp()->asetus("UID")).toString();
    if( arkistopolku.isEmpty() || !QFile::exists(arkistopolku))
        arkistopolku = ArkistohakemistoDialogi::valitseArkistoHakemisto();
    if( arkistopolku.isEmpty())
        return false;

    QDir hakemisto(arkistopolku );
    QString arkistonimi = tilikausi_.arkistoHakemistoNimi();

    if( hakemisto.exists( arkistonimi ) )
    {
        // Jos hakemisto on jo olemassa, poistetaan se
        hakemisto.cd( arkistonimi);
        hakemisto.removeRecursively();
        hakemisto.cdUp();
    }

    hakemisto.mkdir( arkistonimi );
    if(!hakemisto.cd( arkistonimi ))
        return false;

    hakemistoPolku_ = hakemisto.absolutePath();

    hakemisto.mkdir("tositteet");
    hakemisto.mkdir("liitteet");
    hakemisto.mkdir("static");

    if( !kp()->logo().isNull() )
    {
        kp()->logo().save(hakemisto.absoluteFilePath("logo.png"),"PNG");
        logo_ = true;
    }


    // Kopioidaan vakitiedostot
    QFile::copy( ":/arkisto/arkisto.css", hakemisto.absoluteFilePath("static/arkisto.css"));
    QFile::copy( ":/arkisto/jquery.js", hakemisto.absoluteFilePath("static/jquery.js"));
//    QFile::copy( ":/arkisto/ohje.html", hakemisto_.absoluteFilePath("ohje.html"));
//    QFile::copy( ":/pic/kitsas150.png", hakemisto_.absoluteFilePath("statickitupiikki.png"));

    return true;
}

void Arkistoija::arkistoiTositteet()
{
    // Hakee tositeluettelon
    KpKysely* kysely = kpk("/tositteet");
    kysely->lisaaAttribuutti("jarjestys","tosite");
    kysely->lisaaAttribuutti("alkupvm", tilikausi_.alkaa());
    kysely->lisaaAttribuutti("loppupvm", tilikausi_.paattyy());
    connect( kysely, &KpKysely::vastaus, this, &Arkistoija::tositeLuetteloSaapuu );
    kysely->kysy();
}

void Arkistoija::arkistoiRaportit()
{
    raporttilaskuri_ = 6;
    Paivakirja* paivakirja = new Paivakirja(this);
    connect( paivakirja, &Paivakirja::valmis,
             [this] (RaportinKirjoittaja rk) { this->arkistoiRaportti(rk,"paivakirja.html"); } );
    paivakirja->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy() );

    Paakirja* paakirja = new Paakirja(this);
    connect( paakirja, &Paakirja::valmis,
             [this] (RaportinKirjoittaja rk) { this->arkistoiRaportti(rk,"paakirja.html"); } );
    paakirja->kirjoita( tilikausi_.alkaa(), tilikausi_.paattyy() );

    TaseErittelija* erittelija = new TaseErittelija(this);
    connect( erittelija, &TaseErittelija::valmis,
             [this] (RaportinKirjoittaja rk) { this->arkistoiRaportti(rk,"taseerittely.html"); } );
    erittelija->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy());

    TiliKarttaListaaja* tililuettelo = new TiliKarttaListaaja(this);
    connect( tililuettelo, &TiliKarttaListaaja::valmis,
             [this] (RaportinKirjoittaja rk) { this->arkistoiRaportti(rk,"tililuettelo.html"); } );
    tililuettelo->kirjoita(TiliKarttaListaaja::KAYTOSSA_TILIT, tilikausi_,
                           true, true, tilikausi_.paattyy(), false);

    TositeLuettelo* tositeluettelo = new TositeLuettelo(this);
    connect( tositeluettelo, &TositeLuettelo::valmis,
             [this] (RaportinKirjoittaja rk) { this->arkistoiRaportti(rk,"tositeluettelo.html"); } );
    tositeluettelo->kirjoita(tilikausi_.alkaa(), tilikausi_.paattyy(),
                             TositeLuettelo::TositeJarjestyksessa | TositeLuettelo::TulostaKohdennukset
                             | TositeLuettelo::SamaTilikausi | TositeLuettelo::TulostaSummat );


    Tilikausi edellinen = kp()->tilikaudet()->tilikausiPaivalle( tilikausi_.alkaa().addDays(-1) );

    QStringList raportit = kp()->asetukset()->asetus("arkistoraportit").split(",");
    for( auto raportti : raportit) {
        raporttilaskuri_++;
        QString raporttinimi(raportti);
        raporttinimi = raporttinimi.replace(QRegularExpression("\\W"),"").toLower().append(".html");
        Raportoija *raportoija = new Raportoija( raportti, kp()->asetus("kieli"), this);
        connect( raportoija, &Raportoija::valmis,
                 [this, raporttinimi] (RaportinKirjoittaja rk) { this->arkistoiRaportti(rk, raporttinimi); } );
        if( raportoija->onkoTaseraportti()) {
            raportoija->lisaaTasepaiva( tilikausi_.paattyy() );
            if( !edellinen.kausitunnus().isEmpty())
                raportoija->lisaaTasepaiva( edellinen.paattyy());
        } else {
            raportoija->lisaaKausi( tilikausi_.alkaa(), tilikausi_.paattyy());
            if( !edellinen.kausitunnus().isEmpty())
                raportoija->lisaaKausi( edellinen.alkaa(), edellinen.paattyy());
        }
        raporttiNimet_.append( qMakePair(raporttinimi, raportoija->nimi()) );
        raportoija->kirjoita(true, -1);
    }
    arkistoiTilinpaatos();

}

void Arkistoija::arkistoiTilinpaatos()
{
    KpKysely *kysely = kpk( QString("/liitteet/0/TP_%1").arg(tilikausi_.paattyy().toString(Qt::ISODate)) );

    connect( kysely, &KpKysely::vastaus, [this] (QVariant* data)
        { this->arkistoiByteArray("tilinpaatos.pdf", data->toByteArray());  this->raporttilaskuri_--; });
    connect( kysely, &KpKysely::virhe, [this] () { this->raporttilaskuri_--; });

    kysely->kysy();
}

void Arkistoija::arkistoiByteArray(const QString &tiedostonnimi, const QByteArray &array)
{
    QDir hakemisto(hakemistoPolku_);
    QFile tiedosto( hakemisto.absoluteFilePath(tiedostonnimi));
    tiedosto.open( QIODevice::WriteOnly);
    tiedosto.write( array );
    tiedosto.close();

    shaBytes.append(QCryptographicHash::hash( array, QCryptographicHash::Sha256).toHex());
    shaBytes.append(" ");
    shaBytes.append(tiedostonnimi.toLatin1());
    shaBytes.append("\n");
}

void Arkistoija::kirjoitaHash() const
{
    QDir hakemisto(hakemistoPolku_);
    QFile tiedosto( hakemisto.absoluteFilePath( "arkisto.sha256" ));
    tiedosto.open( QIODevice::WriteOnly );
    tiedosto.write( shaBytes );
    tiedosto.close();
}

void Arkistoija::merkitseArkistoiduksi()
{
    kp()->settings()->setValue("arkistopvm/" + kp()->asetus("UID") + "-" + tilikausi_.arkistoHakemistoNimi(),
                               QDateTime::currentDateTime());
    kp()->settings()->setValue("arkistopolku/" + kp()->asetus("UID") + "-" + tilikausi_.arkistoHakemistoNimi(),
                               hakemistoPolku_);
    kp()->settings()->setValue("arkistosha/" + kp()->asetus("UID") + "-" + tilikausi_.arkistoHakemistoNimi(),
                               QString(QCryptographicHash::hash( shaBytes, QCryptographicHash::Sha256).toHex()));

    QModelIndex indeksi = kp()->tilikaudet()->index( kp()->tilikaudet()->indeksiPaivalle(tilikausi_.paattyy()) , TilikausiModel::ARKISTOITU );
    emit kp()->tilikaudet()->dataChanged( indeksi, indeksi );

    emit arkistoValmis( hakemistoPolku_ );

    progressDlg_->deleteLater();
    deleteLater();
}

void Arkistoija::tositeLuetteloSaapuu(QVariant *data)
{
    // Lisätään tositteet luetteloon
    QVariantList lista( data->toList() );

    progressDlg_ = new QProgressDialog(tr("Arkistoidaan kirjanpitoa"), tr("Peruuta"), 0, lista.count() + 50 );

    for( auto tosite : lista ) {
        QVariantMap map = tosite.toMap();
        tositeJono_.append( map );
    }
    arkistoitavaTosite_ = 0;
    arkistoiSeuraavaTosite();
}

void Arkistoija::arkistoiSeuraavaTosite()
{
    // Hae tosite arkistoitavaksi
    int indeksi = arkistoitavaTosite_;

    KpKysely* kysely = kpk(QString("/tositteet/%1").arg( tositeJono_.value(indeksi).id() ));
    connect( kysely, &KpKysely::vastaus,
             [this, indeksi] (QVariant* data) { this->arkistoiTosite(data, indeksi);} );

    kysely->kysy();
}

void Arkistoija::arkistoiTosite(QVariant *data, int indeksi)
{
    // Lisätään ensin liitteet luetteloille
    QVariantMap map = data->toMap();
    int liitenro = 1;
    for( auto liite : map.value("liitteet").toList()) {
        QVariantMap liitemap = liite.toMap();
        QString tnimi = liitemap.value("nimi").toString();
        int liiteid = liitemap.value("id").toInt();
        QString liitenimi = QString("%1-%2-%3_%4.%5")
                .arg( kp()->tilikaudet()->tilikausiPaivalle( map.value("pvm").toDate() ).pitkakausitunnus() )
                .arg( map.value("sarja").toString() )
                .arg( map.value("tunniste").toInt(), 8, 10, QChar('0'))
                .arg( liitenro, 2, 10, QChar('0'))
                .arg( tnimi.mid(tnimi.indexOf('.')+1)  );
        liiteNimet_.insert( liiteid, liitenimi );
        liiteJono_.enqueue( liiteid );
    }
    progressDlg_->setMaximum( progressDlg_->maximum() + map.value("liitteet").toList().count() );


    QString nimi = "tositteet/" + tositeJono_.value(indeksi).tiedostonnimi();

    arkistoiByteArray( nimi + ".html", tosite(map, indeksi) );
    arkistoiByteArray(nimi + ".json", QJsonDocument::fromVariant(map).toJson(QJsonDocument::Indented));
    progressDlg_->setValue( progressDlg_->value() + 1);

    arkistoitavaTosite_++;

    if( arkistoitavaTosite_ < tositeJono_.count())
        arkistoiSeuraavaTosite();
    else if( !liiteJono_.isEmpty() )
        arkistoiSeuraavaLiite();
    else if( !raporttilaskuri_ )
        viimeistele();

}

void Arkistoija::arkistoiSeuraavaLiite()
{
    int liiteid = liiteJono_.dequeue();
    QString tiedosto = liiteNimet_.value(liiteid);
    KpKysely* kysely = kpk(QString("/liitteet/%1").arg(liiteid));
    connect( kysely, &KpKysely::vastaus, [this, tiedosto] (QVariant* data) { this->arkistoiLiite(data, tiedosto);  });
    kysely->kysy();
}

void Arkistoija::arkistoiLiite(QVariant *data, const QString tiedosto)
{
    arkistoiByteArray("liitteet/" + tiedosto, data->toByteArray());
    progressDlg_->setValue(progressDlg_->value() + 1);

    if( arkistoitavaTosite_ < tositeJono_.count())
        arkistoiSeuraavaTosite();
    if( !liiteJono_.isEmpty())
        arkistoiSeuraavaLiite();
    else if( !raporttilaskuri_  && arkistoitavaTosite_ >= tositeJono_.count())
        viimeistele();
}

void Arkistoija::arkistoiRaportti(RaportinKirjoittaja rk, const QString &tiedosto)
{
    QString txt = rk.html(true);
    txt.insert( txt.indexOf("</head>"), "<link rel='stylesheet' type='text/css' href='static/arkisto.css'>");
    txt.insert( txt.indexOf("<body>") + 6, navipalkki( ));

    arkistoiByteArray( tiedosto, txt.toUtf8() );
    raporttilaskuri_--;
    if( !raporttilaskuri_ && liiteJono_.isEmpty() && arkistoitavaTosite_ >= tositeJono_.count())
        viimeistele();
}

void Arkistoija::viimeistele()
{
    kirjoitaHash();

    QDir hakemisto(hakemistoPolku_);
    QFile tiedosto( hakemisto.absoluteFilePath("index.html"));
    tiedosto.open( QIODevice::WriteOnly);
    QTextStream out( &tiedosto );
    out.setCodec("UTF-8");

    out << "<html><meta charset=\"UTF-8\"><head><title>";
    out << kp()->asetus("Nimi") + " arkisto";
    out << "</title><link rel='stylesheet' type='text/css' href='static/arkisto.css'></head><body>";

    out << navipalkki();

    if(logo_)
        out << "<img src=logo.png class=logo>";

    out << "<h1 class=etusivu>" << kp()->asetus("Nimi") << "</h1>";
    out << "<h2 class=etusivu>Kirjanpitoarkisto<br>" ;
    out << tilikausi_.kausivaliTekstina();
    out << "</h2>";

    // Jos tilit on päätetty, tulee linkki myös tilinpäätökseen (pdf)
    out << "<h3>" << tr("Tilinpäätös") << "</h3><ul>";

    if( QFile::exists( hakemisto.absoluteFilePath("tilinpaatos.pdf")  ))
           out << "<li><a href=tilinpaatos.pdf>" << tr("Tilinpäätös") << "</a></li>";

    out   << "<li><a href=taseerittely.html>" << tr("Tase-erittely") << "</a></li></ul>";


    out << "<h3>Kirjanpito</h3>";
    out << "<ul><li><a href=paakirja.html>" << tr("Pääkirja") << "</a></li>";
    out << "<li><a href=paivakirja.html>" << tr("Päiväkirja") << "</a></li>";
    out << "<li><a href=tositeluettelo.html>Tositeluettelo</a></li>";
//    out << "<li><a href=tositepaivakirja.html>" << tr("Tositepäiväkirja") << "</a></li>";
    out << "<li><a href=tililuettelo.html>Tililuettelo</a></li>";
    out << "</ul><h3>Raportit</h3><ul>";


    for( auto rnimi : raporttiNimet_)
        out << "<li><a href='" << rnimi.first << "'>" << rnimi.second << "</a></li>";

    out << "</ul>";


    out << tr("<p class=info>Tämä kirjanpidon sähköinen arkisto on luotu %1 <a href=https://kitsas.fi>Kitsas-ohjelman</a> versiolla %2 <br>")
           .arg(QDate::currentDate().toString(Qt::SystemLocaleDate))
           .arg(qApp->applicationVersion());
    out << tr("Arkiston muuttumattomuus voidaan valvoa sha256-tiivisteellä <code>%1</code> </p>").arg( QString(QCryptographicHash::hash( shaBytes, QCryptographicHash::Sha256).toHex()) );
    if( tilikausi_.paattyy() > kp()->tilitpaatetty() )
        out << "Kirjanpito on viel&auml; keskener&auml;inen.";


    out << "</body></html>";
    merkitseArkistoiduksi();

}

QByteArray Arkistoija::tosite(const QVariantMap& tosite, int indeksi)
{
    QByteArray ba;
    QTextStream out (&ba);
    out.setCodec("utf-8");

    out << "<html><meta charset=\"UTF-8\"><head><title>" << tosite.value("otsikko").toString() << "</title>";
    out << "<link rel='stylesheet' type='text/css' href='../static/arkisto.css'></head><body>";

    out << navipalkki(indeksi);

    const QVariantList& liitteet = tosite.value("liitteet").toList();

    // LIITTEET

    if( liitteet.count() )
    {
        if( indeksi > -1) {
            // Liitteen laatikko, johon nykyinen liite ladataan
            out << "<iframe width='100%' height='50%' class='liite' id='liite' src='../liitteet/";
            out << liiteNimet_.value( liitteet.value(0).toMap().value("id").toInt() );
            out <<  "'></iframe>";
        }

        out << "<table class='liiteluettelo'>";

        // luettelo
        for( auto liite : liitteet) {
            const QVariantMap& liitemap = liite.toMap();
            const QString liitetiedosto = liiteNimet_.value( liitemap.value("id").toInt() );
            out << "<tr><td onclick=\"$('#liite').attr('src','../liitteet/"
                 << liitetiedosto
                 << "');\">" << liitemap.value("nimi").toString()
                 << "</td><td><a href='../liitteet/" << liitetiedosto
                 << "' class=avaaliite>Avaa</a></td></tr>\n";
        }
        out << "</table>";
    }

    // Seuraavaksi otsikot
    out << "<table class=tositeotsikot><tr>";
    out << "<td class=paiva>" << tosite.value("pvm").toDate().toString("dd.MM.yyyy") << "</td>";
    out << "<td class=tositeotsikko>" << tosite.value("otsikko").toString() << "</td>";

    out << QString("<td class=tositetunnus>%1")
           .arg(kp()->tositeTunnus( tosite.value("tunniste").toInt(),
                                    tosite.value("pvm").toDate(),
                                    tosite.value("sarja").toString()));
    out << "</tr></table>";


    // Viennit
    out << "<table class=viennit>";
    out <<  "<tr><th>Pvm</th><th>Tili</th><th>Kohdennus</th><th>Selite</th><th>Debet</th><th>Kredit</th></tr>";

    for( auto vienti : tosite.value("viennit").toList())    {
        QVariantMap vientiMap = vienti.toMap();
        Tili* tili = kp()->tilit()->tili( vientiMap.value("tili").toInt() );
        if( !tili)
            continue;

        out << "<tr><td class=pvm>" << vientiMap.value("pvm").toDate().toString("dd.MM.yyyy") ;
        out << "</td><td class=tili><a href='../paakirja.html#" << tili->numero() << "'>"
            << tili->nimiNumero() << "</a>";

        out << "</td><td class=kohdennus>";
        if( vientiMap.value("kohdennus").toInt())
            out << kp()->kohdennukset()->kohdennus( vientiMap.value("kohdennus").toInt() ).nimi();

        // TODO: Merkkaukset ja tase-erät

        out << "</td><td class=selite>" << vientiMap.value("selite").toString();
        out << "</td><td class=euro>" << ((qAbs(vientiMap.value("debet").toDouble()) > 1e-5) ?  QString("%L1 €").arg(vientiMap.value("debet").toDouble(),0,'f',2) : "");
        out << "</td><td class=euro>" << ((qAbs(vientiMap.value("kredit").toDouble()) > 1e-5) ?  QString("%L1 €").arg(vientiMap.value("kredit").toDouble(),0,'f',2) : "");
        out << "</td></tr>\n";
    }
    out << "</table>";

    // Kommentit
    if( !tosite.value("info").toString().isEmpty())
    {
        out << "<p class=kommentti>";
        out << tosite.value("info").toString().toHtmlEscaped().replace("\n","<br>");
        out << "</p>";
    }

    if( indeksi > -1) {
        out << "<p class=info>Kirjanpito arkistoitu " << QDate::currentDate().toString(Qt::SystemLocaleDate);
        out << "<br><a href=" << tositeJono_.value(indeksi).tiedostonnimi() << ".json>Tositteen t&auml;ydet tiedot</a>";
        out << "<script src='../jquery.js'></script>";
    }
    out << "</body></html>";

    out.flush();
    return ba;

}

QString Arkistoija::tiedostonnimi(const QDate &pvm, const QString &sarja, int tunniste)
{
    return QString("%1-%2-%3")
            .arg( kp()->tilikaudet()->tilikausiPaivalle(pvm).pitkakausitunnus() )
            .arg( sarja )
            .arg( tunniste, 8, 10, QChar('0'));
}

QString Arkistoija::navipalkki(int indeksi) const
{
    QString navi = "<nav><ul><li class=kotinappi><a href=";
    if( indeksi > -1)
        navi.append("../");     // Tositteista palataan päähakemistoon

    navi.append("index.html>");
    if( logo_ )
        navi.append("<img src=logo.png>");
    navi.append( kp()->asetus("Nimi") + " ");

    if( kp()->onkoHarjoitus())
        navi.append("<span class=treeni>HARJOITUS </span>");

    navi.append(tilikausi_.kausivaliTekstina());
    navi.append("</a></li>");


    if( indeksi > -1 && indeksi + 1 < tositeJono_.count() )
        navi.append( tr("<li class=nappi><a href='%1.html'>Seuraava &rarr;</a></li>")
                     .arg( tositeJono_.value(indeksi+1).tiedostonnimi() ) );
    else
        navi.append( "<li class=nappi> </li>");

    if( indeksi > 0 )
        navi.append( tr("<li class=nappi><a href='%1.html'>&larr; Edellinen</a></li>")
                     .arg(tositeJono_.value(indeksi-1).tiedostonnimi()) );
    else
        navi.append("<li class=nappi> </li>");


    navi.append("</ul></nav></div>");

    return navi;
}


Arkistoija::JonoTosite::JonoTosite() :
    tunniste_(0), id_(0)
{

}

Arkistoija::JonoTosite::JonoTosite(const QString &sarja, int tunniste, int id, const QDate &pvm) :
    sarja_(sarja), tunniste_(tunniste), id_(id), pvm_(pvm)
{

}

Arkistoija::JonoTosite::JonoTosite(const QVariantMap &map)
    : sarja_(map.value("sarja").toString()),
      tunniste_( map.value("tunniste").toInt()),
      id_( map.value("id").toInt()),
      pvm_( map.value("pvm").toDate())
{

}

QString Arkistoija::JonoTosite::tiedostonnimi()
{
    qDebug() << "Arkistoija::JonoTosite::tiedostonnimi ";
    qDebug() << pvm().toString() << " " << tunniste();
    return Arkistoija::tiedostonnimi(pvm(), sarja(), tunniste());
}
