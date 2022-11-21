#ifndef GROUPBOOKSMODEL_H
#define GROUPBOOKSMODEL_H

#include <QAbstractTableModel>
#include <QPixmap>

class GroupBooksModel : public QAbstractTableModel
{
    Q_OBJECT


protected:
    class GroupBook {
    public:
        explicit GroupBook();
        GroupBook(const QVariantMap &map);

        int id;
        QString name;
        bool trial;
        QPixmap logo;
    };

public:
    explicit GroupBooksModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);

private:
    QList<GroupBook> books_;
};

#endif // GROUPBOOKSMODEL_H
