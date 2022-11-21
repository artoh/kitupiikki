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

    id_ = map.value("id").toInt();
    name_ = map.value("name").toString();

    const QString typeString = map.value("type").toString();
    if( typeString == "UNIT")
        type_ = GroupNode::UNIT;
    else if( typeString == "GROUP")
        type_ = GroupNode::GROUP;
    else
        type_ = GroupNode::OFFICE;

    books_->load(map.value("books").toList());
    members_->load(map.value("members").toList());

    emit loaded();
}
