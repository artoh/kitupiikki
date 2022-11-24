#include "groupmember.h"
#include "oikeusinfonmuodostaja.h"

GroupMember::GroupMember()
{

}

GroupMember::GroupMember(const QVariantMap &map)
{
    userId_ = map.value("userid").toInt();
    name_ = map.value("name").toString();
    rights_ = map.value("rights").toStringList();
    admin_ = map.value("admin").toStringList();
    email_ = map.value("email").toString();
    phone_ = map.value("phone").toString();
    groupname_ = map.value("groupname").toString();
    groupid_ = map.value("groupid").toInt();
}

QString GroupMember::oikeusInfo() const
{
    QString info = "<html><body>";
    info.append(OikeusInfonMuodostaja::oikeusinfo(rights()));
    info.append(OikeusInfonMuodostaja::oikeusinfo(admin()));
    info.append("</body></html>");
    return info;
}

GroupMember::operator bool() const
{
    return userid() != 0;
}
