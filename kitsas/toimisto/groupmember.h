#ifndef GROUPMEMBER_H
#define GROUPMEMBER_H

#include <QVariantMap>
#include <QDate>

class GroupMember
{
public:
    GroupMember();
    GroupMember(const QVariantMap &map);

    int userid() const { return userId_;}
    QString name() const { return name_;}
    QStringList rights() const { return rights_;}
    QStringList admin() const { return admin_;}
    QDate startDate() const { return startDate_;}
    QDate endDate() const { return endDate_;}
    QString email() const { return email_;}
    QString phone() const { return phone_;}
    QString groupname() const { return groupname_;}
    int groupid() const { return groupid_;}

    QString oikeusInfo() const;

    operator bool() const;

private:
    int userId_ = 0;
    QString name_;
    QStringList rights_;
    QStringList admin_;
    QDate startDate_;
    QDate endDate_;
    QString email_;
    QString phone_;
    QString groupname_;
    int groupid_ = 0;

};

#endif // GROUPMEMBER_H
