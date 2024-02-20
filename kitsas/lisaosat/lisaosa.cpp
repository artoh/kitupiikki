#include "lisaosa.h"

Lisaosa::Lisaosa()
{

}

Lisaosa::Lisaosa(const QVariantMap& data)
{
    id_ = data.value("id").toString();
    nimi_ = Monikielinen(data.value("name").toMap());
    rights_ = data.value("rights").toStringList();
    active_ = data.value("active").toBool();
    system_ = data.value("system").toBool();
}
