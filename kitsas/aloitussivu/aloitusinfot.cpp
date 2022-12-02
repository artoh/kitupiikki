#include "aloitusinfot.h"

AloitusInfot::AloitusInfot()
{

}

QString AloitusInfot::toHtml() const
{
    QString txt;
    for( const auto& info : qAsConst(infot_)) {
        txt.append(info.toHtml());
    }
    return txt;
}

void AloitusInfot::info(const QString &luokka, const QString &otsikko, const QString &teksti, const QString &linkki, const QString kuva, const QString ohjelinkki)
{
    infot_.append( AloitusInfo(luokka, otsikko, teksti, linkki, kuva, ohjelinkki));
}

void AloitusInfot::clear()
{
    infot_.clear();
}

void AloitusInfot::asetaInfot(const QVariantList &data)
{
    infot_.clear();
    for(const auto& item : qAsConst(data)) {
        infot_.append(AloitusInfo(item.toMap()));
    }
}
