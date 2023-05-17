#ifndef LISTANPILVI_H
#define LISTANPILVI_H

#include <QPixmap>
#include "badges.h"

class ListanPilvi
{
public:
    ListanPilvi();

    ListanPilvi(const QVariant& variant);

    int id() const { return id_;}
    QString nimi() const { return nimi_;}
    bool kokeilu() const { return kokeilu_;}
    QByteArray logo() const { return logo_;}
    bool ready() const { return ready_;}
    Badges badges() const { return badges_;}

    void asetaBadget(const QStringList& lista);

protected:
    int id_;
    QString nimi_;
    bool kokeilu_;
    QByteArray logo_;
    bool ready_;
    Badges badges_;

};

#endif // LISTANPILVI_H
