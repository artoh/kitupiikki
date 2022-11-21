#include "groupdata.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

GroupData::GroupData(QObject *parent)
    : QObject{parent},
      books_{new GroupBooksModel(this)},
      members_{new GroupMembersModel(this)}
{

}

void GroupData::load(const int groupId)
{
    KpKysely* kysymys = kp()->pilvi()->kysely(
        QString("%1/groups/%2")
            .arg(kp()->pilvi()->pilviLoginOsoite())
            .arg(groupId)
    );
    if( kysymys ) {
        connect(kysymys, &KpKysely::vastaus, this, &GroupData::dataIn);
        kysymys->kysy();
    }
}

void GroupData::dataIn(QVariant *data)
{
    const QVariantMap map = data->toMap();

    const QVariantMap groupMap = map.value("group").toMap();
    id_ = groupMap.value("id").toInt();
    name_ = groupMap.value("name").toString();

    const QString typeString = groupMap.value("type").toString();
    if( typeString == "UNIT")
        type_ = GroupNode::UNIT;
    else if( typeString == "GROUP")
        type_ = GroupNode::GROUP;
    else
        type_ = GroupNode::OFFICE;

    admin_ = map.value("admin").toStringList();
    books_->load(map.value("books").toList());
    members_->load(map.value("members").toList());

    emit loaded();
}
