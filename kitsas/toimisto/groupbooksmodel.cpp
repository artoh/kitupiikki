#include "groupbooksmodel.h"

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

    return 3;
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
        case TUOTE:
            return owners_ ? book.ownername : book.planname;
        }

        return index.column() == 0 ? book.name: book.planname;
    case Qt::DecorationRole:
        if( index.column() ) return QVariant();
        else if( !book.initialized ) return QIcon(":/pic/lisaa.png");
        else return book.logo;
    case IdRooli:
        return book.id;
    case Qt::ForegroundRole:
        if( book.trial) {
            return QColor(Qt::darkGreen);
        }
        return QVariant();
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

    QByteArray ba = QByteArray::fromBase64( map.value("logo").toByteArray() );
    if( ba.isEmpty())
        logo = QPixmap(":/pic/tyhja.png").scaled(32,32);
    else
        logo = QPixmap::fromImage(QImage::fromData(ba));
}
