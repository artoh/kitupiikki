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

    int id() { return id_;}

    bool isGroup() const { return type_ == GroupNode::GROUP; }
    bool isUnit() const { return type_ == GroupNode::UNIT; }
    bool isOffice() const { return type_ == GroupNode::OFFICE; }

    QStringList adminRights() const { return admin_;}

signals:
    void loaded();

private:
    void dataIn(QVariant* data);

private:
    int id_;
    QString name_;
    GroupNode::GroupType type_;
    QStringList admin_;

    GroupBooksModel* books_;
    GroupMembersModel* members_;


};

#endif // GROUPDATA_H
