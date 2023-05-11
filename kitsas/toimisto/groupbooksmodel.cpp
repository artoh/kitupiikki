#include "groupbooksmodel.h"
#include "aloitussivu/kirjanpitodelegaatti.h"
#include <QIcon>

GroupBooksModel::GroupBooksModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant GroupBooksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NIMI:
            return tr("Nimi");
        case YTUNNUS:
            return tr("Y-tunnus");
        case ALV:
            return tr("ALV valmis");
        case TUOTE:
            return owners_ ? tr("Omistaja") : tr("Tuote");
        }
    }

    return QVariant();
}

int GroupBooksModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return books_.count();
}

int GroupBooksModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 4;
}

QVariant GroupBooksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const GroupBook& book = books_.at(index.row());


    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NIMI:
            return book.name;
        case YTUNNUS:
            return book.businessid;
        case ALV:
            return book.vatInfo();
        case TUOTE:
            return owners_ ? book.ownername : book.planname;
        }
    case Qt::TextAlignmentRole:
        return index.column() == ALV ? (int) (Qt::AlignRight | Qt::AlignVCenter) : (int) (Qt::AlignLeft | Qt::AlignVCenter);
    case IdRooli:
        return book.id;
    case KirjanpitoDelegaatti::LogoRooli:
        return book.logo;
    case KirjanpitoDelegaatti::AlustettuRooli:
        return book.initialized;
    case KirjanpitoDelegaatti::IlmoitusRooli:
        return book.notifications;
    case KirjanpitoDelegaatti::HarjoitusRooli:
        return book.trial;
    case KirjanpitoDelegaatti::OutboxRooli:
        return book.outbox;
    case KirjanpitoDelegaatti::InboxRooli:
        return book.inbox;
    case KirjanpitoDelegaatti::MarkedRooli:
        return book.marked;
    case Qt::DecorationRole:
    {
        if( index.column() != ALV) return QVariant();
        else if( !book.vatDue.isValid()) return QIcon(":/pic/tyhja.png");
        else if( book.vatDue < QDate::currentDate()) return QIcon(":/pic/punainen.png");
        const QDate vrt = book.vatDue.addDays( 0 - book.vatDate.day() );
        if( vrt < QDate::currentDate()) return QIcon(":/pic/oranssi.png");
        if( vrt.addMonths(-1) < QDate::currentDate()) return QIcon(":/pic/keltainen.png");
        return QIcon(":/pic/kaytossa.png");
    }
    case Qt::ForegroundRole:
        return index.column() == ALV && book.vatDue < QDate::currentDate() ? QColor(Qt::red) : QVariant();
    default:
        return QVariant();;
    }


}

void GroupBooksModel::load(const QVariantList &list)
{
    beginResetModel();
    books_.clear();
    for(const auto &item : list) {
        const QVariantMap map = item.toMap();
        books_.append(GroupBook(map));
    }
    owners_ = !books_.value(0).ownername.isEmpty();

    endResetModel();
}

void GroupBooksModel::changePlan(int id, const QString &planName)
{
    for(int i=0; i < rowCount(); i++) {
        if( books_.at(i).id == id) {
            books_[i].planname = planName;
            const QModelIndex index = createIndex(i, TUOTE);
            emit dataChanged(index, index);
            break;
        }
    }
}

GroupBooksModel::GroupBook::GroupBook()
{

}

GroupBooksModel::GroupBook::GroupBook(const QVariantMap &map)
{
    id = map.value("id").toInt();
    name = map.value("name").toString();
    trial = map.value("trial").toBool();
    planname = map.value("plan").toString();
    businessid = map.value("businessid").toString();
    ownername = map.value("ownername").toString();
    initialized = map.value("initialized").toBool();

    notifications = map.value("notifications").toInt();
    inbox = map.value("inbox").toInt();
    outbox = map.value("outbox").toInt();
    marked = map.value("marked").toInt();

    vatDate = map.value("vatdate").toDate();
    vatPeriod = map.value("vatperiod").toInt();
    vatDue = map.value("vatdue").toDate();


    logo = QByteArray::fromBase64( map.value("logo").toByteArray() );
}

QString GroupBooksModel::GroupBook::vatInfo() const
{
    if( vatPeriod == 1)
        return vatDate.toString("M / yy");
    else if( vatPeriod == 3) {
        return QString("Q%1 / %2").arg(vatDate.month() / 3).arg(vatDate.year() % 100);
    } else if( vatPeriod == 12) {
        return QString::number(vatDate.year() % 100);
    } else if(vatDate.isValid()) {
        return vatDate.toString("dd.MM.yyyy");
    }
    return QString();
}
