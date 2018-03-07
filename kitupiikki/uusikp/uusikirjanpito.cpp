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

#include <QDir>

#include <QPixmap>
#include <QIcon>

#include <QFile>
#include <QTextStream>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMapIterator>

#include <QApplication>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <QProgressDialog>

#include <QDesktopServices>
#include <QUrl>

#include "uusikirjanpito.h"

#include "introsivu.h"
#include "nimisivu.h"
#include "tilikarttasivu.h"
#include "tilikausisivu.h"
#include "sijaintisivu.h"
#include "loppusivu.h"

#include "db/kirjanpito.h"
#include "db/asetusmodel.h"
#include "db/tilimodel.h"
#include "db/tili.h"
#include "db/tilikausimodel.h"
#include "db/tositelajimodel.h"

#include "skripti.h"

#include <QDebug>

UusiKirjanpito::UusiKirjanpito() :
    QWizard()
{
    setWindowIcon(QIcon(":/pic/Possu64.png"));
    setWindowTitle("Uuden kirjanpidon luominen");
    setPixmap( WatermarkPixmap, QPixmap(":/pic/Possu64.png") );
    addPage( new IntroSivu());
    addPage( new TilikarttaSivu);
    addPage( new NimiSivu());
    addPage( new TilikausiSivu );
    addPage( new SijaintiSivu );
    addPage( new LoppuSivu );

    setOption( HaveHelpButton, true);
    connect( this, SIGNAL(helpRequested()), this, SLOT(naytaOhje()));
}


QString UusiKirjanpito::aloitaUusiKirjanpito()
{
    UusiKirjanpito velho;

    velho.exec();

    if( velho.alustaKirjanpito())
        // Palautetaan uuden kirjanpidon hakemistopolku
        return velho.field("sijainti").toString() + "/" + velho.field("hakemisto").toString();
    else
        // Epäonnistui, tyhjä merkkijono
        return QString();
}

QMap<QString, QStringList> UusiKirjanpito::lueKtkTiedosto(const QString &polku)
{
    // ktk-tiedosto koostuu osista, jotka merkitään [otsikko] ja
    // niiden väleissä olevista tiedoista. Rivi voidaan
    // kommentoida //-merkillä


    QMap<QString, QStringList> tiedot;

    QFile tiedosto(polku);
    if( tiedosto.open(QIODevice::ReadOnly))
    {
        QTextStream in(&tiedosto);
        in.setCodec("utf8");
        QString nykyavain;
        QStringList nykytieto;

        while( !in.atEnd())
        {
            QString rivi = in.readLine();
            if( rivi.startsWith('[') && rivi.endsWith(']'))
            {
                // Tallennetaan nykyinen
                if( !nykyavain.isEmpty())
                    tiedot[nykyavain] = nykytieto;
                // Aloitetaan uusi
                nykyavain = rivi.mid(1, rivi.length() - 2);
                nykytieto.clear();
            }
            else if( !rivi.startsWith("//") && !nykyavain.isEmpty())
                nykytieto.append(rivi);
        }
        // Tiedoston lopussa päätetään viimeinen tieto
        if( !nykyavain.isEmpty())
            tiedot[nykyavain] = nykytieto;
    }

    return tiedot;
}

void UusiKirjanpito::naytaOhje()
{
    kp()->ohje("aloitus");
}

bool UusiKirjanpito::alustaKirjanpito()
{
    // Ladataan karttatiedosto
    QMap<QString,QStringList> kartta = lueKtkTiedosto( field("tilikartta").toString() );

    // Näyttää QProgressDialogin jotta käyttäjä ei hermostu

    int prosessiluku = 10;
    QProgressDialog progDlg(tr("Luodaan uutta kirjanpitoa"), QString(), 1, prosessiluku);

    progDlg.setMinimumDuration(0);
    progDlg.setValue(0);
    progDlg.setWindowModality(Qt::WindowModal);
    qApp->processEvents();


    QString hakemistopolku = field("sijainti").toString() + "/" + field("hakemisto").toString();
    QDir hakemisto;

    // Luodaan hakemisto
    if( !hakemisto.mkdir(hakemistopolku) || !hakemisto.cd(hakemistopolku))
        return false;

    // Luodaan alihakemistot
    hakemisto.mkdir("liitteet");
    hakemisto.mkdir("arkisto");

    progDlg.setValue( progDlg.value() + 1 );

    // Luodaan tietokanta
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","luonti");
    db.setDatabaseName(hakemistopolku + "/kitupiikki.sqlite");
    if( !db.open())
    {
        return false;
    }
    QSqlQuery query(db);

    progDlg.setValue( progDlg.value() + 1 );

    // Luodaan tietokanta
    // Tietokannan luontikäskyt ovat resurssitiedostossa luo.sql
    QFile sqltiedosto(":/sql/luo.sql");
    sqltiedosto.open(QIODevice::ReadOnly);
    QTextStream in(&sqltiedosto);
    QString sqluonti = in.readAll();
    sqluonti.replace("\n","");
    QStringList sqlista = sqluonti.split(";");

    foreach (QString kysely,sqlista)
    {
        query.exec(kysely);
        qApp->processEvents();
    }

    progDlg.setValue( progDlg.value() + 1 );


    AsetusModel asetukset(&db, 0, true);

    // Kirjataan tietokannan perustietoja

    asetukset.aseta("Nimi", field("nimi").toString());
    asetukset.aseta("Ytunnus", field("ytunnus").toString());
    asetukset.aseta("Harjoitus", field("harjoitus").toBool());

    // Valittu muoto
    if( field("muoto").toString() != "-")
        asetukset.aseta("Muoto", field("muoto").toString());

    asetukset.aseta("Luotu", QDate::currentDate());
    asetukset.aseta("LuotuVersiolla", qApp->applicationVersion());
    asetukset.aseta("KpVersio",  Kirjanpito::TIETOKANTAVERSIO );

    progDlg.setValue( progDlg.value() + 1 );

    // Siirretään asetustauluun tilikartan tiedot
    // jotka alkavat [Isolla kirjaimella]
    QMapIterator<QString,QStringList> i(kartta);
    while( i.hasNext())
    {
        i.next();
        if( !i.key().isEmpty() && i.key().at(0).isUpper() )
        {
            asetukset.aseta( i.key() , i.value());
        }
    }

    progDlg.setValue( progDlg.value() + 1 );

    // Tilien ja otsikkojen kirjoittaminen
    TiliModel tilit(&db);

    QRegularExpression tiliRe("^(?<tyyppi>\\w{1,5})(?<tila>[\\*\\-]?)\\s+(?<nro>\\d{1,8})(\\.\\.(?<asti>\\d{1,8}))?"
                              "\\s*(?<json>\\{.*\\})?\\s(?<nimi>.+)$");

    QStringList tililista = kartta.value("tilit");
    foreach ( QString tilirivi, tililista)
    {
        // Tilitietueet ovat TYYPPI[*-] numero {json} Nimi
        QRegularExpressionMatch mats = tiliRe.match(tilirivi);
        if( mats.hasMatch())
        {
            Tili tili;
            tili.asetaTyyppi( mats.captured("tyyppi"));

            if( mats.captured("tila") == "*")
                tili.asetaTila(2);
            else if( mats.captured("tila") == "-")
                tili.asetaTila(0);
            else
                tili.asetaTila(1);

            tili.asetaNumero( mats.captured("nro").toInt());
            tili.asetaNimi( mats.captured("nimi"));
            tili.json()->fromJson( mats.captured("json").toUtf8());

            if( !mats.captured("asti").isEmpty() )
                tili.json()->set("Asti", mats.captured("asti").toInt());

            if( tili.onko(TiliLaji::PANKKITILI) && !field("iban").toString().isEmpty())
            {
                // #70 Pankkitili jo luontivelhossa
                // Annettu IBAN-numero yhdistetään ensimmäiseen pankkitiliin
                tili.json()->set("IBAN", field("iban").toString().simplified().remove(' '));
                setField("iban",QVariant(""));
            }


            tilit.lisaaTili(tili);
        }

    }

    progDlg.setValue( progDlg.value() + 1 );
    tilit.tallenna(true);
    progDlg.setValue( progDlg.value() + 1 );

    // Tositelajien tallentaminen

    TositelajiModel lajit(&db);

    QStringList lajilista = kartta.value("tositelajit");
    QRegularExpression lajiRe("^(?<tunnus>\\w{1,5})\\s(?<json>\\{.*\\})?\\s(?<nimi>.+)$");

    foreach (QString lajirivi, lajilista)
    {
        QRegularExpressionMatch mats = lajiRe.match(lajirivi);
        if( mats.hasMatch())
        {
            QModelIndex lisatty = lajit.lisaaRivi();
            lajit.setData(lisatty, mats.captured("tunnus"), TositelajiModel::TunnusRooli );
            lajit.setData(lisatty, mats.captured("nimi"), TositelajiModel::NimiRooli);
            lajit.setData(lisatty, mats.captured("json").toUtf8(), TositelajiModel::JsonRooli);
        }
    }
    lajit.tallenna();
    progDlg.setValue( progDlg.value() + 1 );

    // Tilikausien kirjoittaminen
    // Nykyinen tilikausi

    TilikausiModel tilikaudet(&db);
    tilikaudet.lisaaTilikausi( Tilikausi( field("alkaa").toDate(), field("paattyy").toDate() ));

    // Alv-tietojen oletukset
    asetukset.aseta("AlvIlmoitus", field("alkaa").toDate().addDays(-1));
    asetukset.aseta("AlvKausi",1);
    // Laskunumero
    asetukset.aseta("LaskuSeuraavaId",1009);

    if( field("onekakausi").toBool())
    {
        // Ensimmäinen tilikausi, tilinavausta ei tarvita
        asetukset.aseta("Tilinavaus",0);
    }
    else
    {
        // Edellinen tilikausi.
        tilikaudet.lisaaTilikausi( Tilikausi(field("edalkoi").toDate(), field("edpaattyi").toDate()  ) );

        asetukset.aseta("Tilinavaus", 2);
        asetukset.aseta("TilinavausPvm", field("edpaattyi").toDate());

        // #40 Mahdollisuus muokata myös tilinavauskirjausta
        asetukset.aseta("TilitPaatetty", field("edpaattyi").toDate().addDays(-1));
    }

    progDlg.setValue( progDlg.value() + 1 );

    // Kirjoitetaan nollatosite tilien avaamiseen
    if( !field("onekakausi").toBool())
    {
        query.prepare("INSERT INTO TOSITE(id,pvm,otsikko,laji) "
                      "VALUES (0,?,\"Tilinavaus\",0)");
        query.addBindValue( field("edpaattyi").toDate());
        query.exec();
    }
    // Prosessi valmis

    // Yleisskripti
    Skripti::suorita("LuontiSkripti");

    // Muodon aktivoiva skripti
    if( asetukset.onko("Muoto"))
        Skripti::suorita("MuotoOn/" + asetukset.asetus("Muoto"));

    progDlg.setValue( prosessiluku );

    db.close();

    return true;

}
