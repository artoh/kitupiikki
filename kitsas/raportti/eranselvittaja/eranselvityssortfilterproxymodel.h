#ifndef ERANSELVITYSSORTFILTERPROXYMODEL_H
#define ERANSELVITYSSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class EranSelvitysSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit EranSelvitysSortFilterProxyModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

};

#endif // ERANSELVITYSSORTFILTERPROXYMODEL_H
