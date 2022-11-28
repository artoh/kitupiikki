#ifndef GROUPTREEMODEL_H
#define GROUPTREEMODEL_H

#include <QAbstractItemModel>

class GroupNode;

class GroupTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum { IdRole = Qt::UserRole, TypeRole = Qt::UserRole + 1};

    explicit GroupTreeModel(QObject *parent = nullptr);

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addGroup(const int parentId, const QVariantMap &payload );
    void remove(const int id);
    void edit(const int id, const QVariantMap& payload);

    GroupNode* nodeById(const int id) const;

    void refresh();
    int nodes();

private:
    void createTree(const QVariant* data);

    void groupInserted(const int parentId, const QVariant* data);

private:
    GroupNode* rootNode_ = nullptr;
};

#endif // GROUPTREEMODEL_H
