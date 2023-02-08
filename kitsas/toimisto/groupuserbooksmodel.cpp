#include "groupuserbooksmodel.h"

GroupUserBooksModel::GroupUserBooksModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant GroupUserBooksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NIMI:
            return tr("Nimi");
        }
    }

    return QVariant();
}

int GroupUserBooksModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return books_.count();
}

int GroupUserBooksModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 1;
}

QVariant GroupUserBooksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const UserBook& book = books_.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NIMI:
            return book.name;
        }
    case Qt::DecorationRole:
        if( index.column() ) return QVariant();
        return book.logo;
    case IdRooli:
        return book.id;
    default:
        return QVariant();;
    }


    return QVariant();
}

void GroupUserBooksModel::load(const QVariantList &list)
{
    beginResetModel();
    books_.clear();
    for(const auto &item : list) {
        const QVariantMap map = item.toMap();
        books_.append(UserBook(map));
    }
    endResetModel();
}

GroupUserBooksModel::UserBook::UserBook()
{

}

GroupUserBooksModel::UserBook::UserBook(const QVariantMap &map)
{
    id = map.value("id").toInt();
    name = map.value("name").toString();

    QByteArray ba = QByteArray::fromBase64( map.value("logo").toByteArray() );
    if( ba.isEmpty())
        logo = QPixmap(":/pic/tyhja.png").scaled(32,32);
    else
        logo = QPixmap::fromImage(QImage::fromData(ba));

}
