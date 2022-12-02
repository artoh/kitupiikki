#include "palvelunkertoja.h"

PalvelunKertoja::PalvelunKertoja()
{

}

void PalvelunKertoja::setService(const QString &serviceName, const QString &url)
{
    services_.insert(serviceName, url);
}

void PalvelunKertoja::setServices(const QVariantMap &services)
{
    services_ = services;
}

QString PalvelunKertoja::service(const QString &serviceName) const
{
    return services_.value(serviceName).toString();
}
