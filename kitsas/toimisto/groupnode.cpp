#include "groupnode.h"

GroupNode::GroupNode(const QVariantMap& map, GroupNode *parent)
{
    parent_ = parent;
    id_ = map.value("id").toInt();
    name_ = map.value("name").toString();
    const QString& type = map.value("type").toString();
    if( !parent)
        type_ = GroupType::ROOT;
    else if( type == "OFFICE")
        type_ = GroupType::OFFICE;
    else if( type == "UNIT")
        type_ = GroupType::UNIT;
    else
        type_ = GroupType::GROUP;

    const QVariantList subList = map.value("subgroups").toList();
    for(const auto& subItem : subList) {
        const QVariantMap subMap = subItem.toMap();
        children_.append( new GroupNode(subMap, this));
    }
}

GroupNode::~GroupNode()
{
    qDeleteAll(children_);
}

void GroupNode::addChildNode(const QVariantMap &map)
{
    children_.append( new GroupNode(map, this) );
}

GroupNode *GroupNode::findById(const int groupId)
{
    if( groupId == id()) {
        return this;
    }
    for( auto child : children_) {
        GroupNode* node = child->findById(groupId);
        if( node ) {
            return node;
        }
    }
    return nullptr;
}

GroupNode *GroupNode::createNodes(const QVariantList &list)
{
    GroupNode* root = new GroupNode(QVariantMap(), nullptr);
    for(const auto& item: list) {
        const QVariantMap map = item.toMap();
        root->children_.append( new GroupNode(map, root) );
    }
    return root;
}
