#include "grouptreemodel.h"
#include "groupnode.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"

GroupTreeModel::GroupTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &GroupTreeModel::refresh);

    if( kp()->pilvi()->kayttaja()) {
        refresh();
    }
}

QModelIndex GroupTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if( !hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    GroupNode* parentItem =
            parent.isValid() ?
            static_cast<GroupNode*>(parent.internalPointer()) :
            rootNode_;

    GroupNode* childItem = parentItem->subGroup(row);
    if( childItem ) {
        return createIndex(row, column, childItem);
    }
    return QModelIndex();

}

QModelIndex GroupTreeModel::parent(const QModelIndex &index) const
{
    if( !index.isValid() )
        return QModelIndex();

    GroupNode* childItem = static_cast<GroupNode*>(index.internalPointer());
    GroupNode* parentItem = childItem->parent();

    if( parentItem == rootNode_)
        return QModelIndex();

    return createIndex( parentItem->myIndex(), 0, parentItem);
}

int GroupTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0 || !rootNode_)
        return 0;

    GroupNode* parentNode =
            parent.isValid() ?
            static_cast<GroupNode*>(parent.internalPointer()) :
            rootNode_;

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

    GroupNode *node = static_cast<GroupNode*>(index.internalPointer());
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
    case TypeRole:
        return node->type();
    case AdminRightsRole:
        return node->adminRigts();
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

void GroupTreeModel::remove(const int id)
{
    GroupNode* node = rootNode_->findById(id);    
    GroupNode* parent = node->parent();
    const QModelIndex index = createIndex(parent->myIndex(), 0, parent);

    beginRemoveRows(index, node->myIndex(), node->myIndex());
    parent->removeChildNode(node->myIndex());
    delete node;

    KpKysely* kysymys = kp()->pilvi()->loginKysely(
                QString("/groups/%1").arg(id), KpKysely::DELETE);
    kysymys->kysy();
    endRemoveRows();
}

void GroupTreeModel::edit(const int id, const QVariantMap& payload)
{
    GroupNode* node = rootNode_->findById(id);
    node->setName(payload.value("name").toString());
    const QModelIndex index = createIndex(node->myIndex(), 0, node);

    KpKysely* kysymys = kp()->pilvi()->loginKysely(
        QString("/groups/%1").arg(id), KpKysely::PATCH);
    connect( kysymys, &KpKysely::vastaus, this,
             [this, index] { emit this->dataChanged(index,index); });
    kysymys->kysy(payload);
}

GroupNode *GroupTreeModel::nodeById(const int id) const
{
    return rootNode_ ? rootNode_->findById(id) : nullptr;
}

void GroupTreeModel::refresh()
{
    if( kp()->pilvi()->kayttaja()) {
        KpKysely* kysymys = kp()->pilvi()->loginKysely("/groups");
        connect( kysymys, &KpKysely::vastaus, this, &GroupTreeModel::createTree);
        kysymys->kysy();
    }
}

int GroupTreeModel::nodes()
{
    if( rootNode_)
        return rootNode_->nodeCount() - 1;
    else
        return 0;
}

void GroupTreeModel::createTree(const QVariant *data)
{
    beginResetModel();
    const QVariantList lista = data->toList();
    rootNode_ = GroupNode::createNodes(lista);
    endResetModel();
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
