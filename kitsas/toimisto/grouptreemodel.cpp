#include "grouptreemodel.h"
#include "groupnode.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

GroupTreeModel::GroupTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &GroupTreeModel::refresh);
}

QModelIndex GroupTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if( !rootNode_ || row < 0 || column < 0) {
        return QModelIndex();
    }
    GroupNode* parentNode = nodeFromIndex(parent);
    GroupNode* childNode = parentNode->subGroup(row);
    if( !childNode ) {
        return QModelIndex();
    }
    return createIndex(row, column, childNode);

}

QModelIndex GroupTreeModel::parent(const QModelIndex &index) const
{
    GroupNode* node = nodeFromIndex(index);
    if(!node) {
        return QModelIndex();
    }
    GroupNode* parentNode = node->parent();
    if( !parentNode) {
        return QModelIndex();
    }
    GroupNode* grandParentNode = parentNode->parent();
    if( !grandParentNode ) {
        return QModelIndex();
    }

    int row = grandParentNode->indexOf(parentNode);
    return createIndex(row, 0, parentNode);
}

int GroupTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    GroupNode* parentNode = nodeFromIndex(parent);
    if( !parentNode ) {
        return 0;
    }
    return parentNode->subGroupsCount();
}

int GroupTreeModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}

QVariant GroupTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column())
        return QVariant();

    GroupNode *node = nodeFromIndex(index);
    if( !node) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
        return node->name();
    case Qt::DecorationRole:
        if(node->type() == GroupNode::UNIT) return QIcon(":/pic/folder.png");
        if(node->type() == GroupNode::GROUP) return QIcon(":/pic/kansiot.png");
        return QIcon(":/pic/pixaby/toimisto.svg");
    case IdRole:
        return node->id();
    default:
        return QVariant();
    }

}

void GroupTreeModel::addGroup(const int parentId, const QVariantMap& payload)
{
    KpKysely* kysymys = kp()->pilvi()->loginKysely(
        QString("/groups/%2/group").arg(parentId),
        KpKysely::POST
    );
    connect( kysymys, &KpKysely::vastaus, this,
             [this, parentId] (QVariant* data) {this->groupInserted(parentId, data);} );
    kysymys->kysy(payload);
}

void GroupTreeModel::refresh()
{
    if( kp()->pilvi()->kayttaja()) {
        KpKysely* kysymys = kp()->pilvi()->loginKysely("/groups");
        connect( kysymys, &KpKysely::vastaus, this, &GroupTreeModel::createTree);
        kysymys->kysy();
    }
}

void GroupTreeModel::createTree(const QVariant *data)
{
    beginResetModel();
    const QVariantList lista = data->toList();
    rootNode_ = GroupNode::createNodes(lista);
    endResetModel();
}

GroupNode *GroupTreeModel::nodeFromIndex(const QModelIndex &index) const
{
    if( index.isValid() ) {
        return static_cast<GroupNode*>(index.internalPointer());
    } else {
        return rootNode_;
    }
}

void GroupTreeModel::groupInserted(const int parentId, const QVariant *data)
{    

    GroupNode* parentNode = rootNode_->findById(parentId);
    GroupNode* grand = parentNode->parent();
    QModelIndex newIndex = createIndex( grand->indexOf(parentNode), 0, parentNode );

    beginInsertRows(newIndex, parentNode->subGroupsCount(), parentNode->subGroupsCount());
    parentNode->addChildNode(data->toMap());
    endInsertRows();

}
