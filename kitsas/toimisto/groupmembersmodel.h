#ifndef GROUPMEMBERSMODEL_H
#define GROUPMEMBERSMODEL_H

#include <QAbstractTableModel>

#include "groupmember.h"

class GroupMembersModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit GroupMembersModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);
    GroupMember getMember(const int userid) const;

private:
   QList<GroupMember> members_;
};

#endif // GROUPMEMBERSMODEL_H
