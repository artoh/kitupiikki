#ifndef GROUPUSERDATA_H
#define GROUPUSERDATA_H

class GroupUserBooksModel;
class GroupUserMembersModel;

#include <QObject>

class GroupUserData : public QObject
{
    Q_OBJECT
public:
    explicit GroupUserData(QObject *parent = nullptr);

    int id() const;
    const QString &name() const;
    const QString &email() const;
    const QString &phone() const;

    void load(int id);


    GroupUserBooksModel *books() const;
    GroupUserMembersModel *members() const;

signals:
    void loaded();

protected:
    void dataIn(QVariant* data);

private:
    int id_;
    QString name_;
    QString email_;
    QString phone_;

    GroupUserBooksModel* books_;
    GroupUserMembersModel* members_;

};

#endif // GROUPUSERDATA_H
