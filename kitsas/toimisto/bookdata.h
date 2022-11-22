#ifndef BOOKDATA_H
#define BOOKDATA_H

#include <QObject>
#include <QImage>
#include <QDateTime>

class GroupMembersModel;
class AuthLogModel;

class BookData : public QObject
{
    Q_OBJECT
public:
    explicit BookData(QObject *parent = nullptr);

    void load(const int bookId);

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

    bool loginAvailable() const;
    void openBook();

    GroupMembersModel* directUsers() const { return directUsers_;}
    GroupMembersModel* groupUsers() const { return groupUsers_;}
    AuthLogModel* authLog() const { return authLog_; }

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
    int documents_;
    QString size_;

    int planId_;
    QString planName_;


    GroupMembersModel* directUsers_;
    GroupMembersModel* groupUsers_;
    AuthLogModel* authLog_;

};

#endif // BOOKDATA_H
