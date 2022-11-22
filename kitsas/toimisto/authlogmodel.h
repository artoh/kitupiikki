#ifndef AUTHLOGMODEL_H
#define AUTHLOGMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>

class AuthLogModel : public QAbstractTableModel
{
    Q_OBJECT

protected:

    class AuthLogItem {
    public:
        explicit AuthLogItem();
        AuthLogItem(const QVariantMap& map);

        QString name;
        QDateTime last;
        int count;
    };

public:
    enum { NAME, LAST, COUNT};

    explicit AuthLogModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);

private:
    QList<AuthLogItem> log_;
};

#endif // AUTHLOGMODEL_H
