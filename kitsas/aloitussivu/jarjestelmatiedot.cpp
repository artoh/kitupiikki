#include "jarjestelmatiedot.h"

#include <QSysInfo>
#include <QApplication>
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikayttaja.h"

#include "versio.h"

JarjestelmaTiedot::JarjestelmaTiedot(QObject *parent)
    : QAbstractTableModel(parent)
{
    lataaSysteemiTiedot();
    pyydaInfo();
}

int JarjestelmaTiedot::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return tiedot_.count();
}

int JarjestelmaTiedot::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant JarjestelmaTiedot::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Tieto& tieto = tiedot_.at(index.row());
    if( role == Qt::DisplayRole) {
        return index.column() ? tieto.nayttoArvo : tieto.nayttoAvain;
    }

    return QVariant();
}

void JarjestelmaTiedot::lisaa(const QString &avain, const QString &arvo)
{
    beginInsertRows(QModelIndex(), tiedot_.count(), tiedot_.count());
    tiedot_.append(Tieto(avain, arvo));
    endInsertRows();
}

void JarjestelmaTiedot::lisaaPaikallinen(const char *avain, const QString &arvo, bool kaannaArvo)
{
    Tieto tieto(QString::fromUtf8(avain), arvo);
    tieto.nayttoAvain = tr(avain);
    if (kaannaArvo)
        tieto.nayttoArvo = tr(arvo.toUtf8().constData());

    beginInsertRows(QModelIndex(), tiedot_.count(), tiedot_.count());
    tiedot_.append(tieto);
    endInsertRows();
}

void JarjestelmaTiedot::lataaSysteemiTiedot()
{
    PilviKayttaja kayttaja = kp()->pilvi()->kayttaja();
    AvattuPilvi kirjanpito = kp()->pilvi()->pilvi();

    lisaaPaikallinen(QT_TR_NOOP("Versio"), qApp->applicationVersion());
    lisaaPaikallinen(QT_TR_NOOP("Käyttöjärjestelmä"), QSysInfo::prettyProductName());
    lisaaPaikallinen(QT_TR_NOOP("Käyttäjä"), QString("%1 %2").arg( kayttaja.id() ).arg(kayttaja.nimi()));
    lisaaPaikallinen(QT_TR_NOOP("Tilaus"), QString("%1 %2").arg(kayttaja.planId()).arg(kayttaja.planName()));
    lisaaPaikallinen(QT_TR_NOOP("Kirjanpito"), kp()->kirjanpitoPolku());
    if( kirjanpito ) {
        lisaaPaikallinen(QT_TR_NOOP("Tuote"), QString("%1").arg(kirjanpito.planId()));
    }
    lisaaPaikallinen(QT_TR_NOOP("Yritys"), kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi));
    lisaaPaikallinen(QT_TR_NOOP("Tilikartta"), kp()->asetukset()->asetus(AsetusModel::Tilikartta));
    lisaaPaikallinen(QT_TR_NOOP("Tilikartan pvm"), kp()->asetukset()->asetus(AsetusModel::TilikarttaPvm));
    lisaaPaikallinen(QT_TR_NOOP("Muoto"), kp()->asetukset()->asetus(AsetusModel::Muoto));
    lisaaPaikallinen(QT_TR_NOOP("Laajuus"), kp()->asetukset()->asetus(AsetusModel::Laajuus));

    lisaaPaikallinen(QT_TR_NOOP("2FA"), kayttaja.with2FA() ? QT_TR_NOOP("Kyllä") : QT_TR_NOOP("Ei"), true);

    lisaaPaikallinen(QT_TR_NOOP("Kooste"), KITSAS_BUILD);
    QString koostepaiva(__DATE__);      // Tämä päivittyy aina versio.h:ta muutettaessa
    lisaaPaikallinen(QT_TR_NOOP("Käännetty"),QDate::fromString( koostepaiva.mid(4,3) + koostepaiva.left(3) + koostepaiva.mid(6), Qt::RFC2822Date).toString("dd.MM.yyyy"));
#ifdef KITSAS_PORTABLE
    lisaaPaikallinen(QT_TR_NOOP("Portable"),QT_TR_NOOP("Kyllä"), true);
#endif
    lisaaPaikallinen(QT_TR_NOOP("Kitsas Pro"), PRO_VERSIO ? QT_TR_NOOP("Kyllä") : QT_TR_NOOP("Ei"), true);
    lisaaPaikallinen(QT_TR_NOOP("Moodi"), kayttaja.moodi() == PilviKayttaja::PRO ? "Pro" : QT_TR_NOOP("Normaali"), true);

}

QVariantList JarjestelmaTiedot::asList() const
{
    QVariantList list;
    for(const Tieto& tieto : tiedot_) {
        QVariantMap map;
        map.insert("key", tieto.avain());
        map.insert("value", tieto.arvo());
        list.append(map);
    }
    return list;
}



void JarjestelmaTiedot::pyydaInfo()
{
    KpKysely* kysely = kpk("/info");
    if( kysely ) {
        connect( kysely, &KpKysely::vastaus, this, &JarjestelmaTiedot::infoSaapuu);
        kysely->kysy();
    }
}

void JarjestelmaTiedot::infoSaapuu(QVariant *info)
{
    QVariantMap map = info ? info->toMap() : QVariantMap();
    for(const QString& avain : map.keys()) {
        lisaa( avain, map.value(avain).toString());
    }
}

JarjestelmaTiedot::Tieto::Tieto()
{

}

JarjestelmaTiedot::Tieto::Tieto(const QString &avain, const QString &arvo) :
    nayttoAvain(avain), nayttoArvo(arvo), avain_{avain}, arvo_{arvo}
{

}
