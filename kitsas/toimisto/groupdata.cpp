#include "groupdata.h"

#include "groupbooksmodel.h"
#include "groupmembersmodel.h"
#include "shortcutmodel.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "alv/verovarmennetila.h"

#include <QTimer>

GroupData::GroupData(QObject *parent)
    : QObject{parent},
      books_{new GroupBooksModel(this)},
      members_{new GroupMembersModel(this)},
      shortcuts_{ new ShortcutModel(this)},
      varmenneTila_{ new VeroVarmenneTila(this)}
{
    members_->setShortcuts(shortcuts_);
}

void GroupData::load(const int groupId)
{
    if( groupId ) {
        KpKysely* kysymys = kp()->pilvi()->loginKysely(
            QString("/groups/%1").arg(groupId)
        );
        if( kysymys ) {
            connect(kysymys, &KpKysely::vastaus, this, &GroupData::dataIn);
            kysymys->kysy();
        }
    } else {
        QVariant var;
        dataIn(&var);
    }
}

void GroupData::addBook(const QVariant &velhoMap)
{
    KpKysely *kysymys = kp()->pilvi()->loginKysely(
        QString("/groups/%1/book").arg(id()),
        KpKysely::POST
    );
    connect( kysymys, &KpKysely::vastaus, this, [this]
        { this->reload(); kp()->pilvi()->paivitaLista(); });

    kysymys->kysy(velhoMap);
}

void GroupData::deleteMembership(const int userid)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely("/groups/users", KpKysely::DELETE);

    kysymys->lisaaAttribuutti("user", userid);
    kysymys->lisaaAttribuutti("group", id());

    connect( kysymys, &KpKysely::vastaus, this, &GroupData::reload );
    kysymys->kysy();
}

void GroupData::deleteBook(const int bookid)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/books/%1").arg(bookid), KpKysely::DELETE);
    connect( kysymys, &KpKysely::vastaus, this, &GroupData::reload );
    kysymys->kysy();

}

void GroupData::lisaaVarmenne(const QString &siirtotunnus, const QString &salasana)
{
    QVariantMap payload;
    payload.insert("transferid", siirtotunnus);
    payload.insert("password", salasana);

    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/%1/cert").arg(id()), KpKysely::PUT);
    connect( kysymys, &KpKysely::vastaus, this, &GroupData::reload);
    kysymys->kysy(payload);

    QTimer::singleShot(42000, this, &GroupData::reload );
}

void GroupData::poistaVarmenne()
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/%1/cert").arg(id()), KpKysely::DELETE);
    connect( kysymys, &KpKysely::vastaus, this, &GroupData::reload);
    kysymys->kysy();
}

void GroupData::dataIn(QVariant *data)
{
    const QVariantMap map = data->toMap();

    const QVariantMap groupMap = map.value("group").toMap();
    id_ = groupMap.value("id").toInt();
    name_ = groupMap.value("name").toString();
    businessId_ = groupMap.value("businessid").toString();
    shortcuts_->load(groupMap.value("shortcuts").toList());

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

    const QVariantMap officeMap = groupMap.value("office").toMap();
    officeName_ = officeMap.value("name").toString();
    officeType_ = officeMap.value("type").toString();

    officeTypes_ = map.value("officetypes").toList();
    products_ = map.value("products").toList();

    const QVariantMap certMap = map.value("cert").toMap();
    varmenneTila_->set(certMap);

    emit loaded();
}

void GroupData::reload()
{
    if( id() ) {
        load( id());
    }
}
