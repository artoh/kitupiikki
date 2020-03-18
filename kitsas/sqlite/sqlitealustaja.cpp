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
#include "sqlitealustaja.h"
#include "db/tositetyyppimodel.h"
#include "model/tosite.h"

#include <QSqlQuery>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QApplication>
#include <QSqlError>
#include <QJsonDocument>
#include <QDate>
#include <QProgressDialog>

#include "versio.h"

bool SqliteAlustaja::luoKirjanpito(const QString &polku, const QVariantMap &initials)
{
    SqliteAlustaja alustaja;

    QVariantMap initMap = initials.value("init").toMap();

    bool onnistui =  alustaja.alustaTietokanta(polku) &&
            alustaja.teeInit(initMap) &&
            alustaja.lopputoimet() &&
            !alustaja.progress->wasCanceled();

    alustaja.progress->close();

    return onnistui;
}

SqliteAlustaja::SqliteAlustaja() :
    QObject(nullptr)
{
    progress = new QProgressDialog(tr("Alustetaan kirjanpitoa..."), tr("Peruuta"),0,10);
    progress->setMinimumDuration(500);

    db = QSqlDatabase::addDatabase("QSQLITE","uusi");
}

QString SqliteAlustaja::json(const QVariant &var)
{
    return QString::fromUtf8( QJsonDocument::fromVariant(var).toJson(QJsonDocument::Compact) ) ;
}

bool SqliteAlustaja::alustaTietokanta(const QString &polku)
{

    progress->setValue(1);
    qApp->processEvents();

    db.setDatabaseName(polku);
    if( !db.open() ){
        QMessageBox::critical(nullptr, tr("Kirjanpidon %1 luominen epäonnistui").arg(polku), tr("Tietokannan luominen epäonnistui seuraavan virheen takia: %1").arg( db.lastError().text() ));
        return false;
    }
    progress->setValue(2);

    // Kirjanpidon luomisen ajaksi synkronointi pois käytöstä
    db.exec("PRAGMA SYNCHRONOUS = OFF");

    QSqlQuery query(db);

    // Luodaan tietokanta
    // Tietokannan luontikäskyt ovat resurssitiedostossa luo.sql
    QFile sqltiedosto(":/sqlite/luo.sql");
    sqltiedosto.open(QIODevice::ReadOnly);
    QTextStream in(&sqltiedosto);
    in.setCodec("UTF-8");

    QString sqluonti = in.readAll();
    sqluonti.replace("\n","");
    QStringList sqlista = sqluonti.split(";");

    for(QString kysely : sqlista)
    {
        if(!kysely.isEmpty() &&  !query.exec(kysely))
        {
            QMessageBox::critical(nullptr, tr("Kirjanpidon luominen epäonnistui"), tr("Virhe tietokantaa luotaessa: %1 (%2)").arg(query.lastError().text()).arg(kysely) );
            return false;
        }
        qApp->processEvents();
    }
    progress->setValue(4);

    asetusKysely = QSqlQuery(db);
    asetusKysely.prepare("INSERT INTO Asetus(avain,arvo) VALUES(?,?)");
    otsikkoKysely = QSqlQuery(db);
    otsikkoKysely.prepare("INSERT INTO Otsikko(numero,taso,json) VALUES (?,?,?)");
    tiliKysely = QSqlQuery( db );
    tiliKysely.prepare("INSERT INTO Tili(numero,tyyppi,json) VALUES(?,?,?)");
    tilikausiKysely = QSqlQuery( db );
    tilikausiKysely.prepare("INSERT INTO Tilikausi(alkaa,loppuu,json) VALUES (?,?,?)");

    progress->setValue(6);
    return true;
}

void SqliteAlustaja::aseta(const QString &avain, const QVariant &arvo)
{
    asetusKysely.addBindValue(avain);
    if( arvo.toString().isEmpty()) {
        asetusKysely.addBindValue( json(arvo) );
    } else {
        asetusKysely.addBindValue(arvo);
    }
    asetusKysely.exec();
}

bool SqliteAlustaja::lopputoimet()
{
    aseta("LaskuSeuraavaId",100);
    db.exec("PRAGMA SYNCHRONOUS = NORMAL");
    db.exec("PRAGMA JOURNAL_MODE = DELETE");
#ifndef KITSAS_DEVEL
    db.exec("PRAGMA LOCKING_MODE = EXCLUSIVE");
#endif
    progress->setValue(10);
    return true;
}

bool SqliteAlustaja::teeInit(const QVariantMap &initMap)
{
    kirjoitaAsetukset( initMap.value("asetukset").toMap());
    kirjoitaTilit( initMap.value("tilit").toList());
    kirjoitaTilikaudet( initMap.value("tilikaudet").toList() );
    progress->setValue(8);
    return true;
}

void SqliteAlustaja::kirjoitaAsetukset(const QVariantMap &asetukset)
{
    QMapIterator<QString,QVariant> iter(asetukset);
    while( iter.hasNext() ) {
        iter.next();
        aseta( iter.key(), iter.value() );
        qApp->processEvents();
    }
}

void SqliteAlustaja::kirjoitaTilit(const QVariantList &tililista)
{
    for(QVariant var : tililista) {
        QVariantMap map = var.toMap();
        int numero = map.take("numero").toInt();
        QString tyyppi = map.take("tyyppi").toString();
        if( tyyppi.startsWith(QChar('H'))) {
            otsikkoKysely.addBindValue(numero);
            otsikkoKysely.addBindValue( tyyppi.mid(1).toInt() );
            otsikkoKysely.addBindValue( json(map) );
            otsikkoKysely.exec();
        } else {
            tiliKysely.addBindValue(numero);
            tiliKysely.addBindValue(tyyppi);
            tiliKysely.addBindValue( json(map) );
            tiliKysely.exec();
        }
        qApp->processEvents();

    }
}

void SqliteAlustaja::kirjoitaTilikaudet(const QVariantList &kausilista)
{
    if( kausilista.count() > 1)
        kirjoitaAvausTosite( kausilista.first().toMap().value("loppuu").toDate() );

    for( QVariant var : kausilista) {
        QVariantMap map = var.toMap();
        tilikausiKysely.addBindValue( map.take("alkaa").toDate() );
        tilikausiKysely.addBindValue( map.take("loppuu").toDate() );
        tilikausiKysely.addBindValue( json(map) );
        tilikausiKysely.exec();        
    }
}

void SqliteAlustaja::kirjoitaAvausTosite(const QDate &tilinavauspaiva)
{
    QSqlQuery avauskysely = QSqlQuery( db );
    avauskysely.prepare("INSERT INTO Tosite (pvm,tyyppi,tila,tunniste,otsikko) "
                        "VALUES (?,?,?,1,'Tilinavaus') ");
    avauskysely.addBindValue(tilinavauspaiva);
    avauskysely.addBindValue(TositeTyyppi::TILINAVAUS);
    avauskysely.addBindValue(Tosite::KIRJANPIDOSSA);
    avauskysely.exec();
}
