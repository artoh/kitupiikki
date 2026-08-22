/*
   Copyright (C) 2026 Kitsas contributors

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
*/
#include "testdb.h"

#include "db/kirjanpito.h"
#include "db/kohdennus.h"
#include "db/tositetyyppimodel.h"
#include "kieli/kielet.h"
#include "model/tosite.h"
#include "model/tositevienti.h"
#include "postgres/postgresmodel.h"
#include "sqlite/sqlitemodel.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QSqlError>
#include <QSqlQuery>
#include <QtTest>
#include <functional>

class DbParityTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void sqlFiles_sameTables();
    void sqlFiles_sameColumns();
    void sqlFiles_identityIsTheOnlyNormalizedDifference();

    void liveSchema_sameTables();
    void liveSchema_sameColumns();
    void liveSchema_sameSeedRows();

    void binding_jsonAsStringRoundtripsOnBoth();
    void binding_jsonAsByteArrayRejectedOnPostgresJsonb();
    void binding_liiteByteArrayRoundtrips();
    void binding_lastInsertId();

    void constraints_vientiCheck();
    void constraints_kohdennusTyyppi();
    void constraints_cascadeDeleteTosite();
    void constraints_restrictDeleteTiliInUse();
    void constraints_liiteUniqueRooli();
    void constraints_onConflictUpsert();

    void route_asetukset();
    void route_tilit();
    void route_init();
    void route_kohdennukset();
    void route_ryhmat();
    void route_kumppanit();
    void route_tuotteet();
    void route_budjetti();
    void route_vakioviitteet();
    void route_liitteet();
    void route_tositeLogic();
    void route_asiakkaatToimittajatLaskutAlv();

private:
    void vertaa(const QVariant& sqlite, const QVariant& postgres, const QString& konteksti = QString());
    QVariant suoritaMolemmissa(const std::function<QVariant()>& toiminto);

    int lisaaTosite(const QString& otsikko, int tyyppi = TositeTyyppi::TULO,
                    const QVariantList& viennit = {}, int kumppani = 0);
    QByteArray sha256(const QByteArray& data) const;

    TestDb db_;
};

void DbParityTest::initTestCase()
{
    static char appName[] = "dbparity";
    static char *argv[] = {appName, nullptr};
    static int argc = 1;
    new QApplication(argc, argv);

    QVERIFY2(QFile::exists(TestDb::sqliteLuoSqlPolku()),
             qPrintable(TestDb::sqliteLuoSqlPolku()));
    QVERIFY2(QFile::exists(TestDb::postgresLuoSqlPolku()),
             qPrintable(TestDb::postgresLuoSqlPolku()));
    QVERIFY2(QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")),
             "QSQLITE driver is not available");

    Kielet::alustaKielet(QStringLiteral(":/tr/tulkki.json"));
    Kirjanpito::asetaInstanssi(new Kirjanpito(db_.portablePolku()));

    QVERIFY(db_.alusta());
    if (!db_.postgresKaytossa()) {
        const bool vaaditaan = QProcessEnvironment::systemEnvironment()
                                   .contains(QStringLiteral("KITSAS_REQUIRE_POSTGRES"));
        const QString viesti = QStringLiteral("PostgreSQL is not available: %1").arg(db_.postgresVirhe());
        if (vaaditaan)
            QFAIL(qPrintable(viesti));
        qWarning() << viesti;
    }
}

void DbParityTest::cleanupTestCase()
{
    db_.sulje();
}

void DbParityTest::vertaa(const QVariant& sqlite, const QVariant& postgres, const QString& konteksti)
{
    const QVariant a = TestDb::normalisoi(sqlite);
    const QVariant b = TestDb::normalisoi(postgres);
    if (a != b) {
        const QString viesti = QStringLiteral("%1\nSQLite: %2\nPostgres: %3")
                                   .arg(konteksti,
                                        QString::fromUtf8(QJsonDocument::fromVariant(a).toJson()),
                                        QString::fromUtf8(QJsonDocument::fromVariant(b).toJson()));
        QFAIL(qPrintable(viesti));
    }
}

QVariant DbParityTest::suoritaMolemmissa(const std::function<QVariant()>& toiminto)
{
    if (!db_.postgresKaytossa()) {
        QTest::qSkip("PostgreSQL is not available (start docker compose or set KITSAS_PG_* )",
                     __FILE__, __LINE__);
        return {};
    }
    if (!QTest::qVerify(db_.avaaSqlite(), "db_.avaaSqlite()", "", __FILE__, __LINE__))
        return {};
    const QVariant sqlite = toiminto();
    db_.sulje();
    if (!QTest::qVerify(db_.avaaPostgres(), "db_.avaaPostgres()", "", __FILE__, __LINE__))
        return {};
    const QVariant postgres = toiminto();
    db_.sulje();
    vertaa(sqlite, postgres);
    return sqlite;
}

int DbParityTest::lisaaTosite(const QString& otsikko, int tyyppi, const QVariantList& viennit, int kumppani)
{
    QVariantMap map;
    map.insert(QStringLiteral("pvm"), QDate(2019, 3, 1));
    map.insert(QStringLiteral("tyyppi"), tyyppi);
    map.insert(QStringLiteral("tila"), Tosite::KIRJANPIDOSSA);
    map.insert(QStringLiteral("otsikko"), otsikko);
    if (kumppani)
        map.insert(QStringLiteral("kumppani"), kumppani);
    if (!viennit.isEmpty())
        map.insert(QStringLiteral("viennit"), viennit);
    return db_.kysy(QStringLiteral("/tositteet"), KpKysely::POST, map).toMap().value(QStringLiteral("id")).toInt();
}

QByteArray DbParityTest::sha256(const QByteArray& data) const
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

void DbParityTest::sqlFiles_sameTables()
{
    const QString sqliteSql = TestDb::lueTiedosto(TestDb::sqliteLuoSqlPolku());
    const QString pgSql = TestDb::lueTiedosto(TestDb::postgresLuoSqlPolku());
    QCOMPARE(TestDb::luoSqlTaulut(sqliteSql), TestDb::luoSqlTaulut(pgSql));
    QStringList odotetut = TestDb::kTables;
    odotetut.sort(Qt::CaseInsensitive);
    QCOMPARE(TestDb::luoSqlTaulut(sqliteSql), odotetut);
}

void DbParityTest::sqlFiles_sameColumns()
{
    const QVariantMap sqlite = TestDb::luoSqlSarakkeet(TestDb::lueTiedosto(TestDb::sqliteLuoSqlPolku()));
    const QVariantMap postgres = TestDb::luoSqlSarakkeet(TestDb::lueTiedosto(TestDb::postgresLuoSqlPolku()));
    QStringList sqliteKeys = sqlite.keys();
    QStringList postgresKeys = postgres.keys();
    sqliteKeys.sort();
    postgresKeys.sort();
    QCOMPARE(sqliteKeys, postgresKeys);
    for (auto it = sqlite.constBegin(); it != sqlite.constEnd(); ++it)
        QCOMPARE(it.value().toStringList(), postgres.value(it.key()).toStringList());
}

void DbParityTest::sqlFiles_identityIsTheOnlyNormalizedDifference()
{
    QString sqlite = TestDb::normalisoiKaavioSql(TestDb::lueTiedosto(TestDb::sqliteLuoSqlPolku()));
    QString postgres = TestDb::normalisoiKaavioSql(TestDb::lueTiedosto(TestDb::postgresLuoSqlPolku()));
    QCOMPARE(sqlite, postgres);
}

void DbParityTest::liveSchema_sameTables()
{
    const auto toiminto = [this]() -> QVariant {
        QStringList nimet = TestDb::taulut(db_.sql());
        for (QString& n : nimet)
            n = n.toLower();
        nimet.sort();
        return nimet;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::liveSchema_sameColumns()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap map;
        for (const QString& taulu : TestDb::kTables)
            map.insert(taulu.toLower(), TestDb::sarakkeet(db_.sql(), taulu));
        return map;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::liveSchema_sameSeedRows()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap map;
        map.insert(QStringLiteral("kohdennus"),
                   TestDb::dump(db_.sql(), QStringLiteral("SELECT id, tyyppi, json FROM Kohdennus ORDER BY id")));
        map.insert(QStringLiteral("kumppani"),
                   TestDb::dump(db_.sql(), QStringLiteral("SELECT nimi, alvtunnus, json FROM Kumppani ORDER BY id")));
        map.insert(QStringLiteral("iban"),
                   TestDb::dump(db_.sql(), QStringLiteral("SELECT iban FROM KumppaniIban ORDER BY iban")));
        return map;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::binding_jsonAsStringRoundtripsOnBoth()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap kumppani{{QStringLiteral("nimi"), QStringLiteral("Json Oy")},
                             {QStringLiteral("osoite"), QStringLiteral("äöå")}};
        db_.kysy(QStringLiteral("/kumppanit"), KpKysely::POST, kumppani);
        return TestDb::dump(db_.sql(), QStringLiteral(
                                           "SELECT nimi, json FROM Kumppani WHERE nimi='Json Oy'"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::binding_jsonAsByteArrayRejectedOnPostgresJsonb()
{
    if (!db_.postgresKaytossa())
        QSKIP("PostgreSQL is not available (start docker compose or set KITSAS_PG_* )");
    const QByteArray json = QJsonDocument::fromVariant(QVariantMap{{QStringLiteral("x"), 1}})
                                .toJson(QJsonDocument::Compact);
    auto insertByteArray = [&json](QSqlDatabase sql) {
        QSqlQuery q(sql);
        q.prepare(QStringLiteral("INSERT INTO Tositeloki(tosite, tila, data) VALUES (NULL, 100, ?)"));
        q.addBindValue(json);
        return q.exec();
    };

    QVERIFY(db_.avaaSqlite());
    QVERIFY2(insertByteArray(db_.sql()), "SQLite should accept a QByteArray json bind");
    db_.sulje();

    QVERIFY(db_.avaaPostgres());
    QVERIFY2(!insertByteArray(db_.sql()),
             "Postgres jsonb must reject a QByteArray/bytea bind");
    db_.sulje();
}

void DbParityTest::binding_liiteByteArrayRoundtrips()
{
    const QByteArray pdf("%PDF-1.4 test");
    const auto toiminto = [this, pdf]() -> QVariant {
        const int tosite = lisaaTosite(QStringLiteral("Liite"));
        QMap<QString, QString> meta;
        meta.insert(QStringLiteral("Filename"), QStringLiteral("kuitti.pdf"));
        meta.insert(QStringLiteral("Content-type"), QStringLiteral("application/pdf"));
        db_.lahetaTiedosto(QStringLiteral("/liitteet/%1").arg(tosite), pdf, meta);
        return TestDb::dump(db_.sql(), QStringLiteral("SELECT nimi, tyyppi, sha FROM Liite ORDER BY id"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::binding_lastInsertId()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap ryhma{{QStringLiteral("nimi"), QStringLiteral("Hallitus")}};
        return db_.kysy(QStringLiteral("/ryhmat"), KpKysely::POST, ryhma);
    };
    const QVariant tulos = suoritaMolemmissa(toiminto);
    QVERIFY(tulos.toMap().value(QStringLiteral("id")).toInt() > 0);
}

void DbParityTest::constraints_vientiCheck()
{
    auto insertBad = [this]() {
        QSqlQuery q(db_.sql());
        q.prepare(QStringLiteral(
            "INSERT INTO Vienti(rivi,tosite,pvm,tili,debetsnt,kreditsnt) VALUES (1,?,?,?,?,?)"));
        const int tosite = lisaaTosite(QStringLiteral("x"));
        q.addBindValue(tosite);
        q.addBindValue(QDate(2019, 3, 1));
        q.addBindValue(1910);
        q.addBindValue(100);
        q.addBindValue(50);
        return q.exec();
    };
    if (!db_.postgresKaytossa())
        QSKIP("PostgreSQL is not available (start docker compose or set KITSAS_PG_* )");
    QVERIFY(db_.avaaSqlite());
    QVERIFY(!insertBad());
    db_.sulje();
    QVERIFY(db_.avaaPostgres());
    QVERIFY(!insertBad());
    db_.sulje();
}

void DbParityTest::constraints_kohdennusTyyppi()
{
    auto insertBad = [this]() {
        QSqlQuery q(db_.sql());
        return q.exec(QStringLiteral("INSERT INTO Kohdennus(tyyppi, json) VALUES (9, '{}')"));
    };
    if (!db_.postgresKaytossa())
        QSKIP("PostgreSQL is not available (start docker compose or set KITSAS_PG_* )");
    QVERIFY(db_.avaaSqlite());
    QVERIFY(!insertBad());
    db_.sulje();
    QVERIFY(db_.avaaPostgres());
    QVERIFY(!insertBad());
    db_.sulje();
}

void DbParityTest::constraints_cascadeDeleteTosite()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantList viennit;
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 3, 1)},
            {QStringLiteral("tili"), 1910},
            {QStringLiteral("debet"), 10.0},
        });
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 3, 1)},
            {QStringLiteral("tili"), 3000},
            {QStringLiteral("kredit"), 10.0},
        });
        const int tosite = lisaaTosite(QStringLiteral("x"), TositeTyyppi::TULO, viennit);
        QSqlQuery q(db_.sql());
        q.exec(QStringLiteral("DELETE FROM Tosite WHERE id=%1").arg(tosite));
        QVariantMap map;
        q.exec(QStringLiteral("SELECT COUNT(*) FROM Vienti"));
        q.next();
        map.insert(QStringLiteral("vienti"), q.value(0).toInt());
        return map;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::constraints_restrictDeleteTiliInUse()
{
    auto yritaPoistaa = [this]() {
        QVariantList viennit;
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 3, 1)},
            {QStringLiteral("tili"), 1910},
            {QStringLiteral("debet"), 10.0},
        });
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 3, 1)},
            {QStringLiteral("tili"), 3000},
            {QStringLiteral("kredit"), 10.0},
        });
        lisaaTosite(QStringLiteral("x"), TositeTyyppi::TULO, viennit);
        QSqlQuery q(db_.sql());
        return q.exec(QStringLiteral("DELETE FROM Tili WHERE numero=1910"));
    };
    if (!db_.postgresKaytossa())
        QSKIP("PostgreSQL is not available (start docker compose or set KITSAS_PG_* )");
    QVERIFY(db_.avaaSqlite());
    QVERIFY(!yritaPoistaa());
    db_.sulje();
    QVERIFY(db_.avaaPostgres());
    QVERIFY(!yritaPoistaa());
    db_.sulje();
}

void DbParityTest::constraints_liiteUniqueRooli()
{
    auto kaksoiskappale = [this]() {
        const int tosite = lisaaTosite(QStringLiteral("x"));
        QSqlQuery q(db_.sql());
        q.prepare(QStringLiteral("INSERT INTO Liite(tosite,roolinimi,nimi,data) VALUES (?,?,?,?)"));
        q.addBindValue(tosite);
        q.addBindValue(QStringLiteral("logo"));
        q.addBindValue(QStringLiteral("a.png"));
        q.addBindValue(QByteArray("A"));
        q.exec();
        q.prepare(QStringLiteral("INSERT INTO Liite(tosite,roolinimi,nimi,data) VALUES (?,?,?,?)"));
        q.addBindValue(tosite);
        q.addBindValue(QStringLiteral("logo"));
        q.addBindValue(QStringLiteral("b.png"));
        q.addBindValue(QByteArray("B"));
        return q.exec();
    };
    if (!db_.postgresKaytossa())
        QSKIP("PostgreSQL is not available (start docker compose or set KITSAS_PG_* )");
    QVERIFY(db_.avaaSqlite());
    QVERIFY(!kaksoiskappale());
    db_.sulje();
    QVERIFY(db_.avaaPostgres());
    QVERIFY(!kaksoiskappale());
    db_.sulje();
}

void DbParityTest::constraints_onConflictUpsert()
{
    const auto toiminto = [this]() -> QVariant {
        db_.kysy(QStringLiteral("/asetukset"), KpKysely::PATCH,
                 QVariantMap{{QStringLiteral("Nimi"), QStringLiteral("Yksi")}});
        db_.kysy(QStringLiteral("/asetukset"), KpKysely::PATCH,
                 QVariantMap{{QStringLiteral("Nimi"), QStringLiteral("Kaksi")}});
        return db_.kysy(QStringLiteral("/asetukset")).toMap().value(QStringLiteral("Nimi"));
    };
    QCOMPARE(suoritaMolemmissa(toiminto).toString(), QStringLiteral("Kaksi"));
}

void DbParityTest::route_asetukset()
{
    const auto toiminto = [this]() -> QVariant {
        db_.kysy(QStringLiteral("/asetukset"), KpKysely::PATCH,
                 QVariantMap{{QStringLiteral("Kotipaikka"), QStringLiteral("Testilä")}});
        QVariantMap asetukset = db_.kysy(QStringLiteral("/asetukset")).toMap();
        asetukset.remove(QStringLiteral("Avattu"));
        asetukset.remove(QStringLiteral("UID"));
        return asetukset;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_tilit()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap tili;
        tili.insert(QStringLiteral("numero"), 1920);
        tili.insert(QStringLiteral("tyyppi"), QStringLiteral("ARP"));
        tili.insert(QStringLiteral("iban"), QStringLiteral("FI2112345600000785"));
        tili.insert(QStringLiteral("nimi"), QStringLiteral("Toinen tili"));
        db_.kysy(QStringLiteral("/tilit"), KpKysely::PUT, tili);
        return db_.kysy(QStringLiteral("/init")).toMap().value(QStringLiteral("tilit"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_init()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap init = db_.kysy(QStringLiteral("/init")).toMap();
        init.remove(QStringLiteral("kierrot"));
        QVariantMap asetukset = init.value(QStringLiteral("asetukset")).toMap();
        asetukset.remove(QStringLiteral("Avattu"));
        asetukset.remove(QStringLiteral("UID"));
        init.insert(QStringLiteral("asetukset"), asetukset);
        return init;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_kohdennukset()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap kpMap;
        kpMap.insert(QStringLiteral("tyyppi"), Kohdennus::KUSTANNUSPAIKKA);
        kpMap.insert(QStringLiteral("nimi"), QStringLiteral("Hallinto"));
        const QVariantMap luotu = db_.kysy(QStringLiteral("/kohdennukset"), KpKysely::POST, kpMap).toMap();
        QVariantMap projekti;
        projekti.insert(QStringLiteral("tyyppi"), Kohdennus::PROJEKTI);
        projekti.insert(QStringLiteral("kuuluu"), luotu.value(QStringLiteral("id")));
        projekti.insert(QStringLiteral("nimi"), QStringLiteral("Remontti"));
        db_.kysy(QStringLiteral("/kohdennukset"), KpKysely::POST, projekti);
        return db_.kysy(QStringLiteral("/kohdennukset"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_ryhmat()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap ryhma{{QStringLiteral("nimi"), QStringLiteral("VIP")}};
        const QVariantMap luotu = db_.kysy(QStringLiteral("/ryhmat"), KpKysely::POST, ryhma).toMap();
        ryhma.insert(QStringLiteral("nimi"), QStringLiteral("VVIP"));
        db_.kysy(QStringLiteral("/ryhmat/%1").arg(luotu.value(QStringLiteral("id")).toInt()),
                 KpKysely::PUT, ryhma);
        const QVariant lista = db_.kysy(QStringLiteral("/ryhmat"));
        db_.kysy(QStringLiteral("/ryhmat/%1").arg(luotu.value(QStringLiteral("id")).toInt()),
                 KpKysely::DELETE);
        QVariantMap map;
        map.insert(QStringLiteral("ennen"), lista);
        map.insert(QStringLiteral("jalkeen"), db_.kysy(QStringLiteral("/ryhmat")));
        return map;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_kumppanit()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap ryhma{{QStringLiteral("nimi"), QStringLiteral("Asiakkaat")}};
        const int ryhmaId = db_.kysy(QStringLiteral("/ryhmat"), KpKysely::POST, ryhma)
                                .toMap().value(QStringLiteral("id")).toInt();
        QVariantMap asiakas;
        asiakas.insert(QStringLiteral("nimi"), QStringLiteral("Testi Oy"));
        asiakas.insert(QStringLiteral("alvtunnus"), QStringLiteral("FI12345671"));
        asiakas.insert(QStringLiteral("iban"), QVariantList{QStringLiteral("FI2112345600000785")});
        asiakas.insert(QStringLiteral("ryhmat"), QVariantList{ryhmaId});
        asiakas.insert(QStringLiteral("osoite"), QStringLiteral("Katu 1"));
        const QVariantMap luotu = db_.kysy(QStringLiteral("/kumppanit"), KpKysely::POST, asiakas).toMap();
        return db_.kysy(QStringLiteral("/kumppanit/%1").arg(luotu.value(QStringLiteral("id")).toInt()));
    };
    const QVariantMap tulos = suoritaMolemmissa(toiminto).toMap();
    QCOMPARE(tulos.value(QStringLiteral("nimi")).toString(), QStringLiteral("Testi Oy"));
}

void DbParityTest::route_tuotteet()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap tuote;
        tuote.insert(QStringLiteral("nimike"), QStringLiteral("Konsultointi"));
        tuote.insert(QStringLiteral("ahinta"), 100.0);
        db_.kysy(QStringLiteral("/tuotteet"), KpKysely::POST, tuote);
        return db_.kysy(QStringLiteral("/tuotteet"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_budjetti()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap data;
        data.insert(QStringLiteral("0"), QVariantMap{{QStringLiteral("3000"), 1234.56}});
        db_.kysy(QStringLiteral("/budjetti/2019-01-01"), KpKysely::PUT, data);
        return db_.kysy(QStringLiteral("/budjetti/2019-01-01"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_vakioviitteet()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap data;
        data.insert(QStringLiteral("tili"), 1700);
        data.insert(QStringLiteral("kohdennus"), 0);
        data.insert(QStringLiteral("otsikko"), QStringLiteral("Jäsenmaksu"));
        db_.kysy(QStringLiteral("/vakioviitteet"), KpKysely::POST, data);
        return db_.kysy(QStringLiteral("/vakioviitteet"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_liitteet()
{
    const QByteArray png = QByteArray::fromHex("89504e470d0a1a0a");
    const auto toiminto = [this, png]() -> QVariant {
        const int tosite = lisaaTosite(QStringLiteral("Liite"));
        QMap<QString, QString> meta;
        meta.insert(QStringLiteral("Filename"), QStringLiteral("a.png"));
        meta.insert(QStringLiteral("Content-type"), QStringLiteral("image/png"));
        db_.lahetaTiedosto(QStringLiteral("/liitteet/%1").arg(tosite), png, meta);
        return db_.kysy(QStringLiteral("/liitteet?alkupvm=2019-01-01&loppupvm=2019-12-31"));
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_tositeLogic()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap asiakas{{QStringLiteral("nimi"), QStringLiteral("Asiakas Ab")}};
        const int kumppaniId = db_.kysy(QStringLiteral("/kumppanit"), KpKysely::POST, asiakas)
                                   .toMap().value(QStringLiteral("id")).toInt();

        QVariantList viennit;
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 5, 2)},
            {QStringLiteral("tili"), 1700},
            {QStringLiteral("debet"), 124.0},
            {QStringLiteral("tyyppi"), TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS},
            {QStringLiteral("kumppani"), kumppaniId},
            {QStringLiteral("era"), QVariantMap{{QStringLiteral("id"), -1}}},
        });
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 5, 2)},
            {QStringLiteral("tili"), 3000},
            {QStringLiteral("kredit"), 100.0},
            {QStringLiteral("tyyppi"), TositeVienti::MYYNTI + TositeVienti::KIRJAUS},
            {QStringLiteral("kumppani"), kumppaniId},
        });
        QVariantMap tosite;
        tosite.insert(QStringLiteral("pvm"), QDate(2019, 5, 2));
        tosite.insert(QStringLiteral("laskupvm"), QDate(2019, 5, 2));
        tosite.insert(QStringLiteral("tyyppi"), TositeTyyppi::MYYNTILASKU);
        tosite.insert(QStringLiteral("tila"), Tosite::KIRJANPIDOSSA);
        tosite.insert(QStringLiteral("otsikko"), QStringLiteral("Lasku 1"));
        tosite.insert(QStringLiteral("kumppani"), kumppaniId);
        tosite.insert(QStringLiteral("viennit"), viennit);
        db_.kysy(QStringLiteral("/tositteet"), KpKysely::POST, tosite);

        QVariantMap map;
        map.insert(QStringLiteral("tositteet"), db_.kysy(QStringLiteral("/tositteet")));
        map.insert(QStringLiteral("viennit"), db_.kysy(QStringLiteral("/viennit?alkupvm=2019-01-01&loppupvm=2019-12-31")));
        map.insert(QStringLiteral("asiakkaat"), db_.kysy(QStringLiteral("/asiakkaat")));
        map.insert(QStringLiteral("myyntilaskut"), db_.kysy(QStringLiteral("/myyntilaskut")));
        map.insert(QStringLiteral("alv"), db_.kysy(QStringLiteral("/alv")));
        return map;
    };
    suoritaMolemmissa(toiminto);
}

void DbParityTest::route_asiakkaatToimittajatLaskutAlv()
{
    const auto toiminto = [this]() -> QVariant {
        QVariantMap toimittaja{{QStringLiteral("nimi"), QStringLiteral("Toimittaja Oy")}};
        const int kumppaniId = db_.kysy(QStringLiteral("/kumppanit"), KpKysely::POST, toimittaja)
                                   .toMap().value(QStringLiteral("id")).toInt();
        QVariantList viennit;
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 7, 1)},
            {QStringLiteral("tili"), 2960},
            {QStringLiteral("kredit"), 124.0},
            {QStringLiteral("tyyppi"), TositeVienti::OSTO + TositeVienti::VASTAKIRJAUS},
            {QStringLiteral("kumppani"), kumppaniId},
            {QStringLiteral("era"), QVariantMap{{QStringLiteral("id"), -1}}},
        });
        viennit.append(QVariantMap{
            {QStringLiteral("pvm"), QDate(2019, 7, 1)},
            {QStringLiteral("tili"), 4000},
            {QStringLiteral("debet"), 124.0},
            {QStringLiteral("tyyppi"), TositeVienti::OSTO + TositeVienti::KIRJAUS},
            {QStringLiteral("kumppani"), kumppaniId},
        });
        QVariantMap tosite;
        tosite.insert(QStringLiteral("pvm"), QDate(2019, 7, 1));
        tosite.insert(QStringLiteral("laskupvm"), QDate(2019, 7, 1));
        tosite.insert(QStringLiteral("tyyppi"), TositeTyyppi::MENO);
        tosite.insert(QStringLiteral("tila"), Tosite::KIRJANPIDOSSA);
        tosite.insert(QStringLiteral("otsikko"), QStringLiteral("Lasku"));
        tosite.insert(QStringLiteral("kumppani"), kumppaniId);
        tosite.insert(QStringLiteral("viennit"), viennit);
        db_.kysy(QStringLiteral("/tositteet"), KpKysely::POST, tosite);

        QVariantMap map;
        map.insert(QStringLiteral("toimittajat"), db_.kysy(QStringLiteral("/toimittajat")));
        map.insert(QStringLiteral("ostolaskut"), db_.kysy(QStringLiteral("/ostolaskut")));
        map.insert(QStringLiteral("kumppani"),
                   db_.kysy(QStringLiteral("/kumppanit/%1").arg(kumppaniId)));
        return map;
    };
    suoritaMolemmissa(toiminto);
}

QTEST_APPLESS_MAIN(DbParityTest)
#include "tst_dbparity.moc"
