/*
   Copyright (C) 2019 Arto Hyvättinen
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#include "sqlalustaja.h"

#include "db/tositetyyppimodel.h"
#include "model/tosite.h"

#include <QApplication>
#include <QDate>
#include <QFile>
#include <QJsonDocument>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

QString SqlAlustaja::json(const QVariant &var)
{
    return QString::fromUtf8( QJsonDocument::fromVariant(var).toJson(QJsonDocument::Compact) );
}

bool SqlAlustaja::suoritaSqlResurssi(QSqlDatabase db, const QString &resurssi)
{
    QSqlQuery query(db);

    QFile sqltiedosto(resurssi);
    if( !sqltiedosto.open(QIODevice::ReadOnly) ) {
        QMessageBox::critical(nullptr, QObject::tr("Kirjanpidon luominen epäonnistui"),
                              QObject::tr("SQL-resurssia %1 ei voitu avata.").arg(resurssi));
        return false;
    }
    QTextStream in(&sqltiedosto);

    QString sqluonti = in.readAll();
    sqluonti.replace("\n","");
    sqluonti.replace("\r","");
    QStringList sqlista = sqluonti.split(";");

    for(const QString& kysely : sqlista)
    {
        if(!kysely.isEmpty() &&  !query.exec(kysely))
        {
            qWarning() << "SQL-lause " << kysely << " epäonnistui ";
            QMessageBox::critical(nullptr, QObject::tr("Kirjanpidon luominen epäonnistui"),
                                  QObject::tr("Virhe tietokantaa luotaessa: %1 (%2)").arg(query.lastError().text(), kysely) );
            return false;
        }
        qApp->processEvents();
    }
    return true;
}

void SqlAlustaja::aseta(QSqlDatabase db, const QString &avain, const QVariant &arvo)
{
    QSqlQuery asetusKysely(db);
    asetusKysely.prepare("INSERT INTO Asetus(avain,arvo) VALUES(?,?)");
    asetusKysely.addBindValue(avain);
    if( arvo.toString().isEmpty()) {
        asetusKysely.addBindValue( json(arvo) );
    } else {
        asetusKysely.addBindValue(arvo);
    }
    asetusKysely.exec();
}

void SqlAlustaja::kirjoitaAsetukset(QSqlDatabase db, const QVariantMap &asetukset)
{
    QMapIterator<QString,QVariant> iter(asetukset);
    while( iter.hasNext() ) {
        iter.next();
        aseta( db, iter.key(), iter.value() );
        qApp->processEvents();
    }
}

void SqlAlustaja::kirjoitaTilit(QSqlDatabase db, const QVariantList &tililista)
{
    QSqlQuery otsikkoKysely(db);
    otsikkoKysely.prepare("INSERT INTO Otsikko(numero,taso,json) VALUES (?,?,?)");
    QSqlQuery tiliKysely(db);
    tiliKysely.prepare("INSERT INTO Tili(numero,tyyppi,iban,json) VALUES(?,?,?,?)");

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
            tiliKysely.addBindValue( map.take("iban"));
            tiliKysely.addBindValue( json(map) );
            tiliKysely.exec();
        }
        qApp->processEvents();
    }
}

void SqlAlustaja::kirjoitaAvausTosite(QSqlDatabase db, const QDate &tilinavauspaiva)
{
    QSqlQuery avauskysely(db);
    avauskysely.prepare("INSERT INTO Tosite (pvm,tyyppi,tila,tunniste,otsikko) "
                        "VALUES (?,?,?,1,'Tilinavaus') ");
    avauskysely.addBindValue(tilinavauspaiva);
    avauskysely.addBindValue(TositeTyyppi::TILINAVAUS);
    avauskysely.addBindValue(Tosite::KIRJANPIDOSSA);
    avauskysely.exec();
}

void SqlAlustaja::kirjoitaTilikaudet(QSqlDatabase db, const QVariantList &kausilista)
{
    QSqlQuery tilikausiKysely(db);
    tilikausiKysely.prepare("INSERT INTO Tilikausi(alkaa,loppuu,json) VALUES (?,?,?)");

    if( kausilista.count() > 1)
        kirjoitaAvausTosite( db, kausilista.first().toMap().value("loppuu").toDate() );

    for( QVariant var : kausilista) {
        QVariantMap map = var.toMap();
        tilikausiKysely.addBindValue( map.take("alkaa").toDate() );
        tilikausiKysely.addBindValue( map.take("loppuu").toDate() );
        tilikausiKysely.addBindValue( json(map) );
        tilikausiKysely.exec();
    }
}

bool SqlAlustaja::kirjoitaInit(QSqlDatabase db, const QVariantMap &initMap, QProgressDialog *progress)
{
    kirjoitaAsetukset( db, initMap.value("asetukset").toMap());
    kirjoitaTilit( db, initMap.value("tilit").toList());
    kirjoitaTilikaudet( db, initMap.value("tilikaudet").toList() );
    aseta(db, "LaskuSeuraavaId", 100);
    if( progress )
        progress->setValue(8);
    return true;
}
