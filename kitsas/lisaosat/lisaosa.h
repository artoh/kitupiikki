#ifndef LISAOSA_H
#define LISAOSA_H

#include <QVariantMap>
#include "kieli/monikielinen.h"

class Lisaosa
{
public:
    Lisaosa();
    Lisaosa(const QVariantMap& data);

    QString id() const { return id_;}
    Monikielinen nimi() const { return nimi_;}
    QStringList rights() const { return rights_;}
    bool active() const { return active_;}

private:
    QString id_;
    Monikielinen nimi_;
    QStringList rights_;
    bool active_;
};

#endif // LISAOSA_H
