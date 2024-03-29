#ifndef PILVIEXTRA_H
#define PILVIEXTRA_H

#include <QVariantMap>
#include "kieli/monikielinen.h"

class PilviExtra
{
public:
    PilviExtra();
    PilviExtra(const QVariantMap& map);

    int id() const { return id_;}
    bool active() const { return active_;}
    QString name() const { return name_;}
    QString title() const { return title_.teksti();}
    QVariantMap info() const { return info_;}
    QVariantMap status() const { return status_;}
    QString description() const { return description_.teksti();}
    QString statusinfo() const { return statusinfo_.teksti();}
    QString price() const { return price_.teksti();}
    QVariantList actions() const { return actions_;}
    bool inTesting() const { return inTesting_;}

private:
    int id_;
    bool active_;
    QString name_;
    Monikielinen title_;
    Monikielinen description_;
    Monikielinen statusinfo_;
    Monikielinen price_;
    QVariantMap info_;
    QVariantMap status_;
    QVariantList actions_;
    bool inTesting_;
};

#endif // PILVIEXTRA_H
