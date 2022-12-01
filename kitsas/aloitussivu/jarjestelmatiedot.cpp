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
        return index.column() ? tieto.arvo() : tieto.avain();
    }

    return QVariant();
}

void JarjestelmaTiedot::lisaa(const QString &avain, const QString &arvo)
{
    beginInsertRows(QModelIndex(), tiedot_.count(), tiedot_.count());
    tiedot_.append(Tieto(avain, arvo));
    endInsertRows();
}

void JarjestelmaTiedot::lataaSysteemiTiedot()
{
    PilviKayttaja kayttaja = kp()->pilvi()->kayttaja();
    AvattuPilvi kirjanpito = kp()->pilvi()->pilvi();

    lisaa("Versio", qApp->applicationVersion());
    lisaa("Käyttöjärjestelmä", QSysInfo::prettyProductName());
    lisaa("Käyttäjä", QString("%1 %2").arg( kayttaja.id() ).arg(kayttaja.nimi()));
    lisaa("Tilaus", QString("%1 %2").arg(kayttaja.planId()).arg(kayttaja.planName()));
    lisaa("Kirjanpito", kp()->kirjanpitoPolku());
    if( kirjanpito ) {
        lisaa("Tuote", QString("%1").arg(kirjanpito.planId()));
    }
    lisaa("Yritys", kp()->asetukset()->asetus(AsetusModel::OrganisaatioNimi));
    lisaa("Tilikartta", kp()->asetukset()->asetus(AsetusModel::Tilikartta));
    lisaa("Tilikartan pvm", kp()->asetukset()->asetus(AsetusModel::TilikarttaPvm));
    lisaa("Muoto", kp()->asetukset()->asetus(AsetusModel::Muoto));
    lisaa("Laajuus", kp()->asetukset()->asetus(AsetusModel::Laajuus));

    lisaa("2FA", kayttaja.with2FA() ? "Kyllä" : "Ei");

    lisaa("Kooste", KITSAS_BUILD);
    QString koostepaiva(__DATE__);      // Tämä päivittyy aina versio.h:ta muutettaessa
    lisaa("Käännetty",QDate::fromString( koostepaiva.mid(4,3) + koostepaiva.left(3) + koostepaiva.mid(6), Qt::RFC2822Date).toString("dd.MM.yyyy"));
#ifdef KITSAS_PORTABLE
    lisaa("Portable","Kyllä");
#endif
    lisaa("Toffee", TOFFEE_VERSIO ? "Kyllä" : "Ei");
    lisaa("Moodi", kayttaja.moodi() == PilviKayttaja::TOFFEE ? "Toffee" : "Normaali");

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
    avain_{avain}, arvo_{arvo}
{

}
