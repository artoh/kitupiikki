#include "groupuserdata.h"

#include "db/kirjanpito.h"
#include "groupuserbooksmodel.h"
#include "groupusermembersmodel.h"
#include "pilvi/pilvimodel.h"

GroupUserData::GroupUserData(QObject *parent)
    : QObject{parent},
      books_{new GroupUserBooksModel(this)},
      members_{new GroupUserMembersModel(this)}
{

}

int GroupUserData::id() const
{
    return id_;
}

const QString &GroupUserData::name() const
{
    return name_;
}

const QString &GroupUserData::email() const
{
    return email_;
}

const QString &GroupUserData::phone() const
{
    return phone_;
}

void GroupUserData::load(int id)
{
    if( id ) {
        KpKysely* kysymys = kp()->pilvi()->loginKysely(
            QString("/groups/users/%1").arg(id)
        );
        if( kysymys ) {
            connect(kysymys, &KpKysely::vastaus, this, &GroupUserData::dataIn);
            kysymys->kysy();
        }
    }
}

void GroupUserData::dataIn(QVariant *data)
{
    const QVariantMap map = data->toMap();

    name_ = map.value("name").toString();
    id_ = map.value("id").toInt();
    email_ = map.value("email").toString();
    phone_ = map.value("phone").toString();

    books_->load(map.value("books").toList());
    members_->load(map.value("groups").toList());

    emit loaded();
}

GroupUserMembersModel *GroupUserData::members() const
{
    return members_;
}

GroupUserBooksModel *GroupUserData::books() const
{
    return books_;
}
