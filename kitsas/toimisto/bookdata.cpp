#include "bookdata.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "groupmembersmodel.h"
#include "authlogmodel.h"

BookData::BookData(QObject *parent)
    : QObject{parent},
      directUsers_{new GroupMembersModel(this)},
      groupUsers_{new GroupMembersModel(this)},
      authLog_{new AuthLogModel(this)}
{

}

void BookData::load(const int bookId)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/books/%1").arg(bookId)
    );
    if( kysymys ) {
        connect( kysymys, &KpKysely::vastaus, this, &BookData::dataIn);
        kysymys->kysy();
    }
}

bool BookData::loginAvailable() const
{
    const int userid = kp()->pilvi()->kayttaja().id();
    return directUsers_->getMember(userid) || groupUsers_->getMember(userid);
}

void BookData::openBook()
{
    kp()->pilvi()->avaaPilvesta(id());
}

void BookData::setShortcuts(ShortcutModel *shortcuts)
{
    directUsers_->setShortcuts(shortcuts);
    groupUsers_->setShortcuts(shortcuts);
}

void BookData::dataIn(QVariant *data)
{
    const QVariantMap map = data->toMap();

    id_ = map.value("id").toInt();
    trial_ = map.value("trial").toBool();

    const QVariantMap company = map.value("company").toMap();
    companyName_ = company.value("name").toString();
    businessId_ = company.value("businessid").toString();

    const QByteArray ba = QByteArray::fromBase64(company.value("logo").toByteArray());
    logo_ = QImage::fromData(ba);

    const QVariantMap stats = map.value("stats").toMap();
    created_ = stats.value("created").toDateTime();
    modified_ = stats.value("modified").toDateTime();
    documents_ = stats.value("documents").toInt();
    size_ = stats.value("size").toString();

    const QVariantMap plan = map.value("plan").toMap();
    planId_ = plan.value("id").toInt();
    planName_ = plan.value("name").toString();

    directUsers_->load( map.value("permissions").toList() );
    groupUsers_->load( map.value("members").toList() );
    authLog_->load( map.value("log").toList());

    emit loaded();
}
