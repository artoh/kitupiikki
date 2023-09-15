#include "eranselvityssortfilterproxymodel.h"
#include "eranselvityseramodel.h"

EranSelvitysSortFilterProxyModel::EranSelvitysSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{

}

bool EranSelvitysSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex nimiIndeksi = sourceModel()->index(source_row, EranSelvitysEraModel::KUMPPANI, source_parent);
    QModelIndex seliteIndeksi = sourceModel()->index(source_row, EranSelvitysEraModel::SELITE, source_parent);

    return nimiIndeksi.data().toString().contains(filterRegularExpression()) || seliteIndeksi.data().toString().contains(filterRegularExpression());
}
