#ifndef BOOKDATA_H
#define BOOKDATA_H

#include <QObject>
#include <QImage>
#include <QDateTime>

class GroupMembersModel;
class AuthLogModel;
class ShortcutModel;

class BookData : public QObject
{
    Q_OBJECT
public:
    explicit BookData(QObject *parent = nullptr);

    void load(const int bookId);
    void reload();

    int id() const { return id_;}
    bool trial() const { return trial_;}
    QString companyName() const { return companyName_;}
    QString businessId() const { return businessId_;}
    QImage logo() const { return logo_;}

    QDateTime created() const { return created_;}
    QDateTime modified() const { return modified_;}
    int documents() const { return documents_;}
    QString prettySize() const { return size_;}

    int planId() const { return planId_;}
    QString planName() const { return planName_;}

    int ownerId() const { return ownerId_;}
    QString ownerName() const { return ownername_;}

    QString certStatus() const { return certStatus_;}
    QString certInfo() const;

    QString dealOfficeName() const { return dealOfficeName_;}

    bool initialized() const { return initialized_; };

    bool loginAvailable() const;
    void openBook();
    void supportLogin();

    GroupMembersModel* directUsers() const { return directUsers_;}
    GroupMembersModel* groupUsers() const { return groupUsers_;}
    AuthLogModel* authLog() const { return authLog_; }

    void setShortcuts(ShortcutModel* shortcuts);
    void removeRights(const int userid);
    void changePlan(const int planid);



signals:
    void loaded();

private:
    void dataIn(QVariant* data);

private:
    int id_ = 0;
    bool trial_ = false;

    QString companyName_;
    QString businessId_;
    QImage logo_;

    QDateTime created_;
    QDateTime modified_;
    int documents_ = 0;
    QString size_;

    int planId_ = 0;
    QString planName_;

    int ownerId_= 0;
    QString ownername_;

    QString certStatus_;
    bool initialized_ = false;

    QString dealOfficeName_;

    GroupMembersModel* directUsers_;
    GroupMembersModel* groupUsers_;
    AuthLogModel* authLog_;

};

#endif // BOOKDATA_H
