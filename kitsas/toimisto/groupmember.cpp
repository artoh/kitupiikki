#include "groupmember.h"

GroupMember::GroupMember()
{

}

GroupMember::GroupMember(const QVariantMap &map)
{
    userId_ = map.value("userid").toInt();
    name_ = map.value("name").toString();
    rights_ = map.value("rights").toStringList();
    admin_ = map.value("admin").toStringList();
}

GroupMember::operator bool() const
{
    return userid() != 0;
}
