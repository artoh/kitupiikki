#ifndef PALVELUNKERTOJA_H
#define PALVELUNKERTOJA_H

#include <QVariantMap>

class PalvelunKertoja
{
public:
    PalvelunKertoja();    

    void setService(const QString& serviceName, const QString& url);
    void setServices(const QVariantMap& services);
    QString service(const QString& serviceName) const;

protected:
    QVariantMap services_;
};

#endif // PALVELUNKERTOJA_H
