#ifndef GROUPMEMBER_H
#define GROUPMEMBER_H

#include <QVariantMap>

class GroupMember
{
public:
    GroupMember();
    GroupMember(const QVariantMap &map);

    int userid() const { return userId_;}
    QString name() const { return name_;}
    QStringList rights() const { return rights_;}
    QStringList admin() const { return admin_;}

    operator bool() const;

private:
    int userId_ = 0;
    QString name_;
    QStringList rights_;
    QStringList admin_;

};

#endif // GROUPMEMBER_H
