#include "groupusermembersmodel.h"

GroupUserMembersModel::GroupUserMembersModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant GroupUserMembersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NIMI:
            return tr("Nimi");
        }
    }

    return QVariant();
}

int GroupUserMembersModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return members_.count();
}

int GroupUserMembersModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 1;
}

QVariant GroupUserMembersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!

    const UserMember& member = members_.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case NIMI:
            return member.name;
        }
    case IdRooli:
        return member.id;
    default:
        return QVariant();;
    }

}

void GroupUserMembersModel::load(const QVariantList &list)
{
    beginResetModel();
    members_.clear();
    for(const auto &item : list) {
        const QVariantMap map = item.toMap();
        members_.append(UserMember(map));
    }

    endResetModel();
}

GroupUserMembersModel::UserMember::UserMember()
{

}

GroupUserMembersModel::UserMember::UserMember(const QVariantMap &map)
{
    id = map.value("id").toInt();
    name = map.value("name").toString();
}
