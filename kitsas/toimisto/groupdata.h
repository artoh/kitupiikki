#ifndef GROUPDATA_H
#define GROUPDATA_H

#include <QObject>

#include "groupbooksmodel.h"
#include "groupmembersmodel.h"
#include "groupnode.h"

class GroupData : public QObject
{
    Q_OBJECT
public:
    explicit GroupData(QObject *parent = nullptr);

    void load(const int groupId);

    GroupBooksModel* books() { return books_;}
    GroupMembersModel* members() { return members_;}

signals:
    void loaded();

private:
    void dataIn(QVariant* data);

private:
    int id_;
    QString name_;
    GroupNode::GroupType type_;

    GroupBooksModel* books_;
    GroupMembersModel* members_;

};

#endif // GROUPDATA_H
