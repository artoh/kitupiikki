#include "pilviextra.h"

PilviExtra::PilviExtra()
{

}

PilviExtra::PilviExtra(const QVariantMap &map)
{
    id_ = map.value("extraid").toInt();
    active_ = map.value("active").toBool();
    name_ = map.value("name").toString();
    info_ = map.value("info").toMap();
    title_ = Monikielinen(info_.value("title"));
    description_ = Monikielinen(info_.value("description"));
    status_ = map.value("status").toMap();
    actions_ = map.value("actions").toList();
    price_ = Monikielinen(info_.value("price").toMap());
    statusinfo_ = Monikielinen(status_.value("info"));
    inTesting_ = !map.value("public").toBool();
}
