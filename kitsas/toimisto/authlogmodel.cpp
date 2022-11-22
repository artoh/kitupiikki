#include "authlogmodel.h"

AuthLogModel::AuthLogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant AuthLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NAME:
            return tr("Nimi");
        case LAST:
            return tr("Viimeisin");
        case COUNT:
            return tr("Kirjautumisia");
        }
    }
    return QVariant();
}

int AuthLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return log_.count();
}

int AuthLogModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant AuthLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const AuthLogItem item = log_.at(index.row());
    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case NAME:
            return item.name;
        case LAST:
            return item.last.toString("dd.MM.yyyy");
        case COUNT:
            return item.count;
        }
    }

    // FIXME: Implement me!
    return QVariant();
}

void AuthLogModel::load(const QVariantList &list)
{
    beginResetModel();
    log_.clear();
    for(const auto& item : list) {
        log_.append( AuthLogItem(item.toMap()) );
    }
    endResetModel();
}

AuthLogModel::AuthLogItem::AuthLogItem()
{

}

AuthLogModel::AuthLogItem::AuthLogItem(const QVariantMap &map)
{
    name = map.value("name").toString();
    last = map.value("last").toDateTime();
    count = map.value("count").toInt();
}
