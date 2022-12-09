#include "pilvikayttaja.h"
#include <QVariantMap>
#include "db/kirjanpito.h"
#include <QSettings>

PilviKayttaja::PilviKayttaja() :
    moodi_(versio__)
{

}

PilviKayttaja::PilviKayttaja(const QVariant &data)
{
    const QVariantMap map = data.toMap();

    id_ = map.value("id").toInt();
    nimi_ = map.value("name").toString();
    email_ = map.value("email").toString();
    phone_ = map.value("phone").toString();
    cloudCount_ = map.value("cloudcount").toInt();
    admin_ = map.value("admin").toBool();

    moodi_ = map.value("mode").toString() == "TOFFEE" ?
                PRO :
                NORMAALI;

    setServices( map.value("services").toMap());
    with2FA_ = map.value("with2fa").toBool();


    const QVariantMap planMap = map.value("plan").toMap();

    plan_id_ = planMap.value("id").toInt();
    plan_name_ = planMap.value("name").toString();        
    extraMonthly_ = planMap.value("extramonthly").toString();
    capacity_ = planMap.value("capacity").toInt();


    const QString sulku = map.value("blocked").toString();
    if( sulku == "UNPAID")
        blocked_ = MAKSAMATON;
    else if( sulku == "RULES")
        blocked_ = EHTOJEN_VASTAINEN;
    else
        blocked_ = KAYTOSSA;

    if( map.contains("key")) {
        QVariantMap keyMap = map.value("key").toMap();
        kp()->settings()->setValue("AuthKey",
             keyMap.value("id").toString()+","+keyMap.value("secret").toString());
    }

}

PilviKayttaja::operator bool() const
{
    return id_ != 0 && blocked_ == KAYTOSSA;
}


void PilviKayttaja::asetaVersioMoodi(const KayttajaMoodi versio)
{
    versio__ = versio;
}


PilviKayttaja::KayttajaMoodi PilviKayttaja::versio__ = PilviKayttaja::NORMAALI;
