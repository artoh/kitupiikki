#include "lisaosasortproxymodel.h"
#include "lisaosalistmodel.h"

LisaosaSortProxyModel::LisaosaSortProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{

}

bool LisaosaSortProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    bool leftActive = sourceModel()->data(left, LisaosaListModel::AktiivinenRooli).toBool();
    bool rightActive = sourceModel()->data(right, LisaosaListModel::AktiivinenRooli).toBool();

    if(leftActive != rightActive)
        return leftActive ? true : false;

    return QSortFilterProxyModel::lessThan(left, right);
}
