#include "bookdata.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

#include "groupmembersmodel.h"
#include "authlogmodel.h"
#include "booknotificationmodel.h"

BookData::BookData(QObject *parent)
    : QObject{parent},
      directUsers_{new GroupMembersModel(this)},
      groupUsers_{new GroupMembersModel(this)},
      authLog_{new AuthLogModel(this)},
      notifications_{new BookNotificationModel(this)}
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

QString BookData::certInfo() const
{
    if( certStatus_ == "OFFICE_OK") return tr("Toimiston varmenne käytössä");
    else if(certStatus_ == "OFFICE_FAIL 1040") return tr("Tilitoimistolta puuttuu Suomi.fi -valtuutus");
    else if(certStatus_ == "OFFICE_FAIL VAT101") return tr("Tarkasta asiakkaan arvonlisäverovelvollisuus");
    else if(certStatus_ == "OFFICE_FAIL 1103") return tr("Virheellinen Y-tunnus");
    else if(certStatus_ == "OFFICE_FAIL 1005") return tr("VeroAPI-yhteydessä virhe");
    else if(certStatus_.startsWith("OFFICE_FAIL")) return tr("Toimiston varmenne ei kelpaa");
    else if(certStatus_ == "NO_CERT") return tr("Ei varmennetta");
    else if(certStatus_ == "BOOK_OK") return tr("Oma varmenne käytössä");
    else if(certStatus_ == "NO_BID") return tr("Y-tunnus puuttuu");
    else if(certStatus_ == "BOOK_PG") return tr("Varmenteen hakeminen kesken");
    else if(certStatus_ == "BOOK_PR") return tr("Varmenteen uusiminen käynnissä");
    else if(certStatus_ == "BOOK_CR") return tr("Varmenne poistettu");
    else if(certStatus_ == "BOOK_ES" || certStatus_ == "BOOK_EG") return tr("Varmenteen hakeminen epäonnistui");
    else return certStatus_;
}


bool BookData::loginAvailable() const
{
    const int userid = kp()->pilvi()->kayttaja().id();
    if( initialized() ) {
        return directUsers_->getMember(userid) || groupUsers_->getMember(userid);
    } else {
        // Jos ei ole vielä alustettu, edellytetään asetusoikeutta jotta voi kirjautua
        return  (directUsers_->getMember(userid) && directUsers_->getMember(userid).rights().contains("As"))  ||
                (groupUsers_->getMember(userid) && groupUsers_->getMember(userid).rights().contains("As"));
    }
}

void BookData::openBook()
{
    kp()->pilvi()->avaaPilvesta(id());
}

void BookData::supportLogin()
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/login/%1").arg(id()));
    connect( kysymys, &KpKysely::vastaus, this, [] (QVariant* vastaus) { kp()->pilvi()->alustaPilvi(vastaus, false); });
    kysymys->kysy();
}

QString BookData::vatString() const
{

    if( !vatDate_.isValid()) {
        return QString();
    } else if( vatPeriod_ == 1)
        return vatDate_.toString("M / yy");
    else if( vatPeriod_ == 3) {
        return QString("Q%1 / %2").arg(vatDate_.month() / 3).arg(vatDate_.year() % 100);
    } else if( vatPeriod_ == 12) {
        return QString::number(vatDate_.year() );
    } else {
        return vatDate_.toString("dd.MM.yyyy");
    }

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

    const QVariantMap ownerMap = map.value("owner").toMap();
    ownerId_ = ownerMap.value("id").toInt();
    ownername_ = ownerMap.value("name").toString();

    const QVariantMap vatMap = map.value("vat").toMap();
    vatDate_ = vatMap.value("date").toDate();
    vatPeriod_ = vatMap.value("period").toInt();
    vatDue_ = vatMap.value("duedate").toDate();

    int groupid = map.value("group").toMap().value("id").toInt();    

    directUsers_->load( map.value("permissions").toList() );
    groupUsers_->load( map.value("members").toList(), groupid );
    authLog_->load( map.value("log").toList());
    notifications_->load( map.value("notifications").toList());

    certStatus_ = map.value("cert").toString();
    initialized_ = map.value("initialized", true).toBool();
    dealOfficeName_ = map.value("dealoffice").toMap().value("name").toString();


    emit loaded();
}



