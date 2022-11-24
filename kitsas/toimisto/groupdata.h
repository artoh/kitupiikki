#ifndef GROUPDATA_H
#define GROUPDATA_H

#include <QObject>


#include "groupnode.h"

class GroupBooksModel;
class GroupMembersModel;
class ShortcutModel;

class GroupData : public QObject
{
    Q_OBJECT
public:
    explicit GroupData(QObject *parent = nullptr);

    void load(const int groupId);
    void reload();

    GroupBooksModel* books() { return books_;}
    GroupMembersModel* members() { return members_;}
    ShortcutModel* shortcuts() { return shortcuts_;}

    int id() const { return id_;}
    QString name() const { return name_; }
    QString officeName() const { return officeName_;}
    QString businessId() const { return businessId_; }
    QString officeType() const { return officeType_;}

    bool isGroup() const { return type_ == GroupNode::GROUP; }
    bool isUnit() const { return type_ == GroupNode::UNIT; }
    bool isOffice() const { return type_ == GroupNode::OFFICE; }

    QStringList adminRights() const { return admin_;}
    QVariantList officeTypes() const { return officeTypes_; }
    QVariantList products() const { return products_;}    

    void addBook(const QVariant& velhoMap);

signals:
    void loaded();

private:
    void dataIn(QVariant* data);    

private:
    int id_;
    QString name_;
    QString businessId_;
    GroupNode::GroupType type_;
    QString officeType_;
    QStringList admin_;

    QString officeName_;
    QVariantList officeTypes_;
    QVariantList products_;

    GroupBooksModel* books_;
    GroupMembersModel* members_;
    ShortcutModel* shortcuts_;


};

#endif // GROUPDATA_H
