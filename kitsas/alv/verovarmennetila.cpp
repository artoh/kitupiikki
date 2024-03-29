#include "verovarmennetila.h"
#include <QVariantMap>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "alvilmoitustenmodel.h"
#include "alvkaudet.h"

VeroVarmenneTila::VeroVarmenneTila(QObject *parent)
    : QObject{parent}
{

}

void VeroVarmenneTila::paivita()
{
    if( kp()->pilvi()->service("vero").isEmpty() )
        return;

    QString url = QString("%1/cert").arg( kp()->pilvi()->service("vero") );
    KpKysely* kysymys = kpk(url);
    if( kysymys ) {
        connect( kysymys, &KpKysely::vastaus, this, &VeroVarmenneTila::tilaSaapuu);
        kysymys->kysy();
    }
}

void VeroVarmenneTila::tyhjenna()
{
    status_.clear();
    errorCode_.clear();
    officeName_.clear();
    officeBid_.clear();
    emit paivitetty();
}

void VeroVarmenneTila::set(const QVariantMap &map)
{
    status_ = map.value("status").toString();
    errorCode_ = map.value("errorcode").toString();
    statusTime_ = map.value("statustime").toDateTime();

    QVariantMap omap = map.value("office").toMap();
    officeName_ = omap.value("name").toString();
    officeBid_ = omap.value("businessid").toString();
}

QString VeroVarmenneTila::toString() const
{
    if( status().isEmpty()) {
        return tr("Varmennetta ei ole otettu käyttöön.");
    } else if( status() == "OK" && statusTime_.daysTo(QDateTime::currentDateTime()) < 3) {
        return tr("Verohallinnon varmenne on noudettu %1.\nVarmenne ei välttämättä ole vielä käytettävissä").arg(statusTime_.toString("dd.MM.yyyy"));
    } else if( status() == "OK") {
        return tr("Verohallinnon varmenne on käytettävissä. \nVarmenne noudettu %1.").arg(statusTime_.toString("dd.MM.yyyy"));
    } else if( status() == "OF") {
        return tr("Tilitoimiston varmenne on käytettävissä") +
                  QString("\n%1 (%2)").arg(officeName_, officeBid_);
    } else if( status() == "PG") {
        return tr("Varmenteen hakeminen on kesken.\nHakeminen kestää noin minuutin.");
    } else if( status() == "PR") {
        return tr("Varmenteen uusiminen on käynnissä.");
    } else if( status() == "CR") {
        return tr("Varmenne on poistettu");
    } else if( errorCode()=="PKI020" ) {
        return tr("Varmenteen hakeminen epäonnistui. \nVarmenteen hakemisessa käytetyt tunnukset olivat virheellisiä.");
    } else {
        return tr("Varmenteen hakeminen epäonnistui (Virhe %1)").arg(errorCode());
    }
}

QString VeroVarmenneTila::information() const
{
    if( kp()->alvIlmoitukset()->kaudet()->onko()) {
        return toString();
    }
    const QString errorCode = kp()->alvIlmoitukset()->kaudet()->virhe();
    if( errorCode == "VAT101") {
        return tr("Tarkasta arvonlisäverovelvolliseksi rekisteröityminen");
    } else if( errorCode == "1103") {
        return tr("Virheellinen y-tunnus");
    } else if( errorCode == "1005") {
        return tr("Verokausien hakeminen epäonnistui.");
    }

    if( status() == "OK") {
        if( errorCode == "1040") {
            return tr("Varmenteelta puuttuu oikeudet verotietoihin");
        } else if( statusTime_.daysTo(QDateTime::currentDateTime()) < 2 ) {
            return tr("Verohallinnon varmenne on noudettu %1.\nVarmenne ei vielä ole käytettävissä.").arg(statusTime_.toString("dd.MM.yyyy"));
        } else {
            return tr("Varmenteessa on ongelma, eikä sillä voi antaa alv-ilmoitusta.");
        }
    } else if( status() == "OF") {
        if( errorCode == "1040") {
            return tr("Tilitoimistolta puuttuu Suomi.fi-valtuutus veroasioiden hoitamiseen") +
                QString("\n%1 (%2)").arg(officeName_, officeBid_);
        }
        return tr("Arvonlisäverokausien hakeminen toimiston varmenteella epäonnistui") +
                QString("\n%1 (%2)").arg(officeName_, officeBid_);
    } else {
        return toString();
    }
}

void VeroVarmenneTila::tilaSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    set(map);

    emit paivitetty();

    if( isValid()) emit kaytossa();
}
