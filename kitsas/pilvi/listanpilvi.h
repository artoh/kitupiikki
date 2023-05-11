#ifndef LISTANPILVI_H
#define LISTANPILVI_H

#include <QPixmap>

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
    int notifications() const { return notifications_;}
    int inbox() const { return inbox_;}
    int outbox() const { return outbox_;}
    int marked() const { return marked_;}

protected:
    int id_;
    QString nimi_;
    bool kokeilu_;
    QByteArray logo_;
    bool ready_;
    int notifications_;
    int inbox_;
    int outbox_;
    int marked_;

};

#endif // LISTANPILVI_H
