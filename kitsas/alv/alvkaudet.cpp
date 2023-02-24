#include "alvkaudet.h"
#include <QVariantMap>

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"


AlvKaudet::AlvKaudet(QObject *parent)
    : QObject{parent}
{

}

void AlvKaudet::haeKaudet()
{
    kaudet_.clear();
    errorCode_.clear();
    QDate pvm = kp()->tilitpaatetty().addDays(1);
    while( pvm.year() <= kp()->paivamaara().year() ) {
        haussa_++;
        QString url = QString("%1/info/periods/%2")
                .arg( kp()->pilvi()->service("vero"),
                      pvm.toString("yyyy") );
        KpKysely* kysymys = kpk(url);
        connect( kysymys, &KpKysely::vastaus, this, &AlvKaudet::saapuu);
        kysymys->kysy();

        pvm = pvm.addYears(1);
    }
}

void AlvKaudet::tyhjenna()
{
    kaudet_.clear();
    emit haettu();
}

AlvKausi AlvKaudet::kausi(const QDate &date) const
{
    for( const auto& kausi : qAsConst(kaudet_)) {
        if( kausi.loppupvm() == date) return kausi;
    }
    return AlvKausi();
}

QList<AlvKausi> AlvKaudet::kaudet() const
{
    return kaudet_;
}

bool AlvKaudet::onko()
{
    return !kaudet_.isEmpty();
}

QString AlvKaudet::virhe() const
{
    if( kaudet_.isEmpty())
        return errorCode_;
    else
        return QString();
}

void AlvKaudet::saapuu(QVariant *data)
{
    QVariantList list = data->toList();

    if(list.isEmpty()) {
        QVariantMap map = data->toMap();
        if(map.contains("ErrorCode")) {
            errorCode_ = map.value("ErrorCode").toString();
        }
    } else {
        for(const auto& item : qAsConst(list)) {
            QVariantMap iMap = item.toMap();
            kaudet_.append(AlvKausi(iMap));
        }
    }
    haussa_--;
    if( haussa_ < 1 && !kaudet_.isEmpty()) {
        std::sort(kaudet_.begin(), kaudet_.end(), AlvKaudet::descSort);
        varmenneTila = OK;
        emit haettu();
    }
}

bool AlvKaudet::descSort(const AlvKausi &a, const AlvKausi &b)
{
    return b.loppupvm() < a.loppupvm();
}

AlvKausi::AlvKausi() :
    tila_{EIKAUTTA}
{
}

AlvKausi::AlvKausi(const QVariantMap &map)
{
    alkupvm_ = map.value("StartDate").toDate();
    loppupvm_ = map.value("EndDate").toDate();
    erapvm_ = map.value("DueDate").toDate();

    QString status = map.value("Status").toString();
    if( status == "Processed") tila_ = KASITELTY;
    else if(status == "Being Processed") tila_ = KASITTELYSSA;
    else if(status == "Missing") tila_ = PUUTTUVA;
    else if(status == "Estimated") tila_ = ARVIOITU;
    else if(status == "Expired") tila_ = ERAANTYNYT;
    else tila_ = EIKAUTTA;

}

QString AlvKausi::tilaInfo() const
{
    switch(tila()) {
        case KASITELTY: return AlvKaudet::tr("Käsitelty");
        case KASITTELYSSA: return AlvKaudet::tr("Käsittelyssä");
        case PUUTTUVA: return AlvKaudet::tr("Ilmoitus puuttuu");
        case ARVIOITU: return AlvKaudet::tr("Arvioitu");
        case ERAANTYNYT: return AlvKaudet::tr("Erääntynyt");
        default: return QString();
    }
}
