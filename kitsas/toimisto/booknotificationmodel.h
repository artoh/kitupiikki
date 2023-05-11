#ifndef BOOKNOTIFICATIONMODEL_H
#define BOOKNOTIFICATIONMODEL_H

#include <QAbstractListModel>


class BookDataNotification {
public:
    enum NotificationType {NOTIFY, INFO, ERROR};

    BookDataNotification(const QString type = QString(), const QString& title = QString());


    NotificationType type() const { return type_;}
    QString title() const { return title_;}

private:
    NotificationType type_ = NOTIFY;
    QString title_;
};


class BookNotificationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit BookNotificationModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);

private:
    QList<BookDataNotification> data_;
};

#endif // BOOKNOTIFICATIONMODEL_H
