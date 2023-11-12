#ifndef LISAOSASORTPROXYMODEL_H
#define LISAOSASORTPROXYMODEL_H

#include <QSortFilterProxyModel>

class LisaosaSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LisaosaSortProxyModel(QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

#endif // LISAOSASORTPROXYMODEL_H
