#ifndef GROUPUSERBOOKSMODEL_H
#define GROUPUSERBOOKSMODEL_H

#include <QAbstractTableModel>
#include <QPixmap>

class GroupUserBooksModel : public QAbstractTableModel
{
    Q_OBJECT


    class UserBook {
    public:
        explicit UserBook();
        UserBook(const QVariantMap& map);

        int id;
        QString name;
        QPixmap logo;
    };

public:
    enum { IdRooli = Qt::UserRole};
    enum { NIMI };

    explicit GroupUserBooksModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);

private:
    QList<UserBook> books_;
};

#endif // GROUPUSERBOOKSMODEL_H
