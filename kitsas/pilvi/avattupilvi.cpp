#include "avattupilvi.h"
#include "db/yhteysmodel.h"

#include <QDebug>
#include <stdexcept>
#include <QJsonDocument>

#include "kieli/monikielinen.h"

AvattuPilvi::AvattuPilvi()
{

}

AvattuPilvi::AvattuPilvi(const QVariant &data)
{
    const QVariantMap map = data.toMap();

    id_ =  map.value("id").toInt();
    bookId_ = map.value("bookId").toString();
    name_ = map.value("name").toString();
    businessid_ = map.value("businessid").toString();
    trial_ = map.value("trial").toBool();
    oikeudet_ = oikeudetListasta(map.value("rights").toList());
    url_ = map.value("url").toString();
    token_ = map.value("token").toString();
    setServices(map.value("services").toMap());
    alustettu_ = map.value("initialized",true).toBool();
    active_ = map.value("active", true).toBool();
    alias_ = map.value("alias").toString();

    const QVariantMap planMap = map.value("plan").toMap();
    plan_id_ = planMap.value("id").toInt();
    vat_ = planMap.value("vat").toBool();
    trial_period_ = planMap.value("trial").toBool();

    QVariantList ekstraLista = map.value("extras").toList();
    for(auto const& item : ekstraLista) {
        PilviExtra extra(item.toMap());
        extrat_.insert(extra.id(), extra);
    }

    asetaNotifikaatiot( map.value("notifications").toList() );

}

AvattuPilvi::operator bool() const
{
    return id() != 0;
}

void AvattuPilvi::asetaNimi(const QString &nimi)
{
    name_ = nimi;
}

void AvattuPilvi::asetaYTunnus(const QString &ytunnus)
{
    businessid_ = ytunnus;
}

void AvattuPilvi::asetaAlias(const QString &alias)
{
    alias_ = alias;
}

PilviExtra AvattuPilvi::extra(int id) const
{
    return extrat_.value(id);
}

QList<int> AvattuPilvi::extrat() const
{
    return extrat_.keys();
}

void AvattuPilvi::asetaNotifikaatiot(const QVariantList &lista)
{
    clear();
    for(auto const& item : lista) {
        const QVariantMap map = item.toMap();

        // TODO: Tyylit sisällön mukaan
        const QString type = map.value("type").toString();
        const QVariantMap im = map.value("info").toMap();
        const QString image = im.contains("image") ? im.value("image").toString() : type == "ERROR" ? "ilmoitus-punainen.svg" : type == "INFO" ? "ilmoitus-keltainen.svg" : "ilmoitus-vihrea.svg";
        const QString notifyClass = im.contains("class") ? im.value("class").toString() : "notify";

        Monikielinen text( im.value("info") );
        Monikielinen title( im.value("title"));

        info(notifyClass, title.teksti(), text.teksti(),
             im.value("link").toString(), image, im.value("help").toString(),
             map.value("id").toString());
    }

}



qlonglong AvattuPilvi::oikeudetListasta(const QVariantList &lista)
{
    qlonglong bittikartta = 0;
    for(const auto& oikeus : lista) {
        try {
            bittikartta |= oikeustunnukset__.at(oikeus.toString());
        } catch( std::out_of_range )
        {
            qWarning() << "Tuntematon oikeus " << oikeus.toString();
        }
    }
    return bittikartta;
}


std::map<QString,qlonglong> AvattuPilvi::oikeustunnukset__ = {
    {"Ts", YhteysModel::TOSITE_SELAUS},
    {"Tl", YhteysModel::TOSITE_LUONNOS},
    {"Tt", YhteysModel::TOSITE_MUOKKAUS},
    {"Ls", YhteysModel::LASKU_SELAUS},
    {"Ll", YhteysModel::LASKU_LAATIMINEN},
    {"Lt", YhteysModel::LASKU_LAHETTAMINEN},
    {"Kl", YhteysModel::KIERTO_LISAAMINEN},
    {"Kt", YhteysModel::KIERTO_TARKASTAMINEN},
    {"Kh", YhteysModel::KIERTO_HYVAKSYMINEN},
    {"Av", YhteysModel::ALV_ILMOITUS},
    {"Bm", YhteysModel::BUDJETTI},
    {"Tp", YhteysModel::TILINPAATOS},
    {"As", YhteysModel::ASETUKSET},
    {"Ko", YhteysModel::KAYTTOOIKEUDET},
    {"Om", YhteysModel::OMISTAJA},
    {"Xt", YhteysModel::TUOTTEET},
    {"Xr", YhteysModel::RYHMAT},
    {"Ra", YhteysModel::RAPORTIT},
    {"Ks", YhteysModel::KIERTO_SELAAMINEN},
    {"Tk", YhteysModel::TOSITE_KOMMENTTI},
    {"Ao", YhteysModel::LISAOSA_ASETUKSET},
    {"Ad", YhteysModel::LISAOSA_KAYTTO},
    {"Ab", YhteysModel::PERUSASETUKSET},
    {"Pm", YhteysModel::MAKSETTAVAKSI},
    {"Pl", YhteysModel::MAKSETTAVAT},
    {"O", YhteysModel::HALLINTA},
    {"OT", YhteysModel::HALLINTA},
    {"OB", YhteysModel::HALLINTA},
    {"OC", YhteysModel::HALLINTA},
    {"OD", YhteysModel::HALLINTA},
    {"OG", YhteysModel::HALLINTA},
    {"OR", YhteysModel::HALLINTA},
    {"OU", YhteysModel::HALLINTA},
    {"OL", YhteysModel::HALLINTA},
    {"OP", YhteysModel::HALLINTA},
    {"OA", YhteysModel::HALLINTA},
    {"A", YhteysModel::HALLINTA},
    {"AT", YhteysModel::HALLINTA},
    {"AO", YhteysModel::HALLINTA},
    {"AS", YhteysModel::HALLINTA},
    {"AP", YhteysModel::HALLINTA},
    {"AK", YhteysModel::HALLINTA},
    {"R", YhteysModel::KIRJANPITAJA},
    {"RT", YhteysModel::TOSITE},
    {"RL", YhteysModel::LASKU},
    {"RK", YhteysModel::KIERTO},
    {"RO", YhteysModel::TYOKALUT},
    {"O", YhteysModel::KPHALLINTA},
};
