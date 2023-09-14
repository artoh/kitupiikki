#include "maksutapa.h"

Maksutapa::Maksutapa()
{

}

Maksutapa::Maksutapa(const QVariantMap &map) 
{
    QMapIterator<QString,QVariant> i(map);
    while(i.hasNext()) {
        i.next();
        tekstit_.insert(i.key(), i.value().toString());
    }
}

QString Maksutapa::nimi(const QString &kieli) const
{
    return teksti(kieli);
}

int Maksutapa::tili() const
{
    return tekstit_.value("TILI").toInt();
}

bool Maksutapa::uusiEra() const
{
    return tekstit_.value("ERA").toInt();
}

QString Maksutapa::kuva() const
{
    return tekstit_.value("KUVA");
}

QString Maksutapa::muotoehto() const
{
    return tekstit_.value("MUOTO");
}

QVariantMap Maksutapa::toMap() const
{
    return map();
}

void Maksutapa::asetaTili(const int tili)
{
    tekstit_.insert("TILI", QString::number(tili));
}

void Maksutapa::asetaUusiEra(bool uusiEra)
{
    if( uusiEra)
        tekstit_.insert("ERA","-1");
    else
        tekstit_.remove("ERA");
}

void Maksutapa::asetaKuva(const QString &kuva)
{
    tekstit_.insert("KUVA", kuva);
}
