#include "groupmembersmodel.h"
#include <QIcon>

#include "shortcutmodel.h"

GroupMembersModel::GroupMembersModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant GroupMembersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    return QVariant();
}

int GroupMembersModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return members_.count();
}

int GroupMembersModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return shortcuts_ ? 2 : 1;
}

QVariant GroupMembersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const GroupMember& member = members_.at(index.row());

    if( index.column() == SHORTCUT) {
        if( role == Qt::DisplayRole && shortcuts_) {
            return shortcuts_->nameFor( member.rights(), member.admin() );
        }
        return QVariant();
    }


    switch (role) {
    case Qt::DisplayRole:
        return member.name();
    case Qt::DecorationRole:
        return( member.admin().isEmpty() ? QIcon(":/pic/mies.png") : QIcon(":/pic/yrittaja.png") );
    default:
        return QVariant();
    }



}

void GroupMembersModel::load(const QVariantList &list)
{
    beginResetModel();
    members_.clear();
    for(const auto& item : list) {
        members_.append(GroupMember(item.toMap()));
    }
    endResetModel();
}

GroupMember GroupMembersModel::getMember(const int userid) const
{
    for(auto member : members_) {
        if( member.userid() == userid) {
            return member;
        }
    }
    return GroupMember();
}

void GroupMembersModel::setShortcuts(ShortcutModel *shortcuts)
{
    shortcuts_ = shortcuts;
}
