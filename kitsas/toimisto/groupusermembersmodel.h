#ifndef GROUPUSERMEMBERSMODEL_H
#define GROUPUSERMEMBERSMODEL_H

#include <QAbstractTableModel>

class GroupUserMembersModel : public QAbstractTableModel
{
    Q_OBJECT

protected:
    class UserMember {
    public:
        explicit UserMember();
        UserMember(const QVariantMap& map);

        int id;
        QString name;
    };

public:
    enum { NIMI };
    enum { IdRooli = Qt::UserRole };

    explicit GroupUserMembersModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const QVariantList& list);

private:
    QList<UserMember> members_;
};

#endif // GROUPUSERMEMBERSMODEL_H
