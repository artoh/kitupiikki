#ifndef GROUPBOOKSMODEL_H
#define GROUPBOOKSMODEL_H

#include "qdatetime.h"
#include <QAbstractTableModel>
#include <QPixmap>

#include "pilvi/badges.h"

class GroupBooksModel : public QAbstractTableModel
{
    Q_OBJECT


protected:
    class GroupBook {
    public:
        explicit GroupBook();
        GroupBook(const QVariantMap &map);
        QString vatInfo() const;

        int id;
        QString name;
        bool trial;
        QByteArray logo;
        QString planname;
        QString businessid;
        QString ownername;
        bool initialized;

        Badges badges_;

        QDate vatDate;
        int vatPeriod;
        QDate vatDue;
    };

public:
    enum { IdRooli = Qt::UserRole };
    enum { NIMI, YTUNNUS, ALV, TUOTE};

    explicit GroupBooksModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);

    void changePlan(int id, const QString& planName);

private:
    QList<GroupBook> books_;
    bool owners_ = false;
};

#endif // GROUPBOOKSMODEL_H
