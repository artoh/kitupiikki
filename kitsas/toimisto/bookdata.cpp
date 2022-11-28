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

void BookData::reload()
{
    load(id());
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

void BookData::supportLogin()
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/login/%1").arg(id()));
    connect( kysymys, &KpKysely::vastaus,
             kp()->pilvi(), &PilviModel::alustaPilvi);
    kysymys->kysy();
}

void BookData::setShortcuts(ShortcutModel *shortcuts)
{
    directUsers_->setShortcuts(shortcuts);
    groupUsers_->setShortcuts(shortcuts);
}

void BookData::removeRights(const int userid)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely("/groups/users", KpKysely::DELETE);

    kysymys->lisaaAttribuutti("user", userid);
    kysymys->lisaaAttribuutti("book", id());

    connect( kysymys, &KpKysely::vastaus, this, &BookData::reload );
    kysymys->kysy();
}

void BookData::changePlan(const int planid)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/books/%1").arg(id()), KpKysely::PATCH);
    QVariantMap payload;
    payload.insert("product", planid);

    connect( kysymys, &KpKysely::vastaus, this, &BookData::reload );
    kysymys->kysy(payload);
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

    int groupid = map.value("group").toMap().value("id").toInt();

    directUsers_->load( map.value("permissions").toList() );
    groupUsers_->load( map.value("members").toList(), groupid );
    authLog_->load( map.value("log").toList());

    emit loaded();
}
