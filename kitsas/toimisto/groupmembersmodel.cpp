#include "groupmembersmodel.h"
#include <QIcon>

#include "shortcutmodel.h"

GroupMembersModel::GroupMembersModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant GroupMembersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case NAME:
            return tr("Nimi");
        case SHORTCUT:
            return tr("Oikeudet");
        case ORIGIN:
            return tr("Ryhmästä");
        }
    }

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

    return groupid_ ? 3 : 2;
}

QVariant GroupMembersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const GroupMember& member = members_.at(index.row());                                                                 

    switch (role) {
    case Qt::DisplayRole:
        switch( index.column() ) {
        case NAME:
            return member.name();
        case SHORTCUT:
            return shortcuts_ ? shortcuts_->nameFor( member.rights(), member.admin() ) : QVariant();
        case ORIGIN:
            return ( member.groupid() && member.groupid() != groupid_  ) ? member.groupname() : QVariant();
    }
    case Qt::DecorationRole:
        if( index.column() == NAME)
            return( member.admin().isEmpty() && !member.rights().contains("Om") ? QIcon(":/pic/mies.png") : QIcon(":/pic/yrittaja.png") );
        return QVariant();
    case Qt::ForegroundRole:
        if( (member.startDate().isValid() && member.startDate() > QDate::currentDate()) ||
            (member.endDate().isValid() && member.endDate() < QDate::currentDate()) ) {
            return QColor(Qt::gray);
        } else {
            return QVariant();
        }
    case IdRooli:
        return member.userid();
    default:
        return QVariant();
    }



}

void GroupMembersModel::load(const QVariantList &list, const int groupid)
{
    groupid_ = groupid;
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
