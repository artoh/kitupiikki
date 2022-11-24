#include "shortcutmodel.h"

ShortcutModel::ShortcutModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ShortcutModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return shortcuts_.count();
}

QVariant ShortcutModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Shortcut data = shortcuts_.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.name();
    case RightsRole:
        return data.rights();
    case AdminRole:
        return data.admin();
    default:
        return QVariant();
    }
}

void ShortcutModel::load(const QVariantList &list)
{
    beginResetModel();
    shortcuts_.clear();
    shortcuts_.append(Shortcut());
    for(const auto& item : list) {
        shortcuts_.append( Shortcut(item.toMap()) );
    }
    endResetModel();
}

int ShortcutModel::indexFor(const QStringList &rights, const QStringList &admin) const
{
    for(int i=1; i < shortcuts_.count(); i++) {
        const Shortcut& shortcut = shortcuts_.at(i);
        if( shortcut.rights() == rights && shortcut.admin() == admin ) return i;
    }
    return 0;
}

QString ShortcutModel::nameFor(const QStringList &rights, const QStringList admin) const
{
    for(int i=1; i < shortcuts_.count(); i++) {
        const Shortcut& shortcut = shortcuts_.at(i);
        if( shortcut.rights() == rights && shortcut.admin() == admin ) return shortcut.name();
    }
    return tr("Muokattu");
}

void ShortcutModel::set(const QString& name, const QStringList &rights, const QStringList &admin, int i)
{
    if(i <= 0 || i >= shortcuts_.count()) {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        shortcuts_.append(Shortcut(name, rights, admin));
        endInsertRows();
    } else {
        shortcuts_.replace(i, Shortcut(name, rights, admin));
        emit dataChanged(index(i), index(i));
    }
}

void ShortcutModel::poista(int indeksi)
{
    beginRemoveRows(QModelIndex(),indeksi, indeksi);
    shortcuts_.removeAt(indeksi);
    endRemoveRows();
}

ShortcutModel::Shortcut::Shortcut()
{
    name_ = ShortcutModel::tr("Muokatut oikeudet");
}

ShortcutModel::Shortcut::Shortcut(const QString &name, const QStringList &rights, const QStringList &admin) :
    name_{name}, rights_{rights}, admin_{admin}
{

}

ShortcutModel::Shortcut::Shortcut(const QVariantMap &map)
{
    name_ = map.value("name").toString();
    rights_ = map.value("rights").toStringList();
    admin_ = map.value("admin").toStringList();
}
