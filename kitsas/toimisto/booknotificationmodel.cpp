#include "booknotificationmodel.h"
#include <QIcon>

BookNotificationModel::BookNotificationModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BookNotificationModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return data_.count();
}

QVariant BookNotificationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const BookDataNotification& item = data_.at(index.row());

    if( role == Qt::DisplayRole) {
        return item.title();
    } else if( role == Qt::DecorationRole) {
        switch (item.type()) {
        case BookDataNotification::ERROR:
            return QIcon(":/pic/ilmoitus-punainen.svg");
        case BookDataNotification::INFO:
            return QIcon(":/pic/ilmoitus-sininen.svg");
        default:
            return QIcon(":/pic/ilmoitus-vihrea.svg");
        }
    }


    // FIXME: Implement me!
    return QVariant();
}

void BookNotificationModel::load(const QVariantList &list)
{
    beginResetModel();
    data_.clear();
    for(const auto& item: list) {
        const QVariantMap map = item.toMap();
        data_.append( BookDataNotification( map.value("type").toString(), map.value("title").toString()) );
    }
    endResetModel();

}

BookDataNotification::BookDataNotification(const QString type, const QString &title) :
    title_(title)
{
    if( type == "ERROR") type_ = ERROR;
    else if(type == "INFO") type_ = INFO;
    else type_ = NOTIFY;
}
