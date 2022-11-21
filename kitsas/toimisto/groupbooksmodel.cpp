#include "groupbooksmodel.h"

GroupBooksModel::GroupBooksModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant GroupBooksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
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

    return 1;
}

QVariant GroupBooksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const GroupBook& book = books_.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return book.name;
    case Qt::DecorationRole:
        return book.logo;
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
    endResetModel();
}

GroupBooksModel::GroupBook::GroupBook()
{

}

GroupBooksModel::GroupBook::GroupBook(const QVariantMap &map)
{
    id = map.value("id").toInt();
    name = map.value("name").toString();
    QByteArray ba = QByteArray::fromBase64( map.value("logo").toByteArray() );
    if( ba.isEmpty())
        logo = QPixmap(":/pic/tyhja.png").scaled(32,32);
    else
        logo = QPixmap::fromImage(QImage::fromData(ba));
}
