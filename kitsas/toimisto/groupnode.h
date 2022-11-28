#ifndef GROUPNODE_H
#define GROUPNODE_H

#include <QList>
#include <QVariantMap>

class GroupNode
{
public:
    enum GroupType { UNIT, GROUP, OFFICE, ROOT} ;

    GroupNode(const QVariantMap &map, GroupNode* parent = nullptr);
    ~GroupNode();

    int id() const { return id_;}
    QString name() const { return name_;}
    GroupType type() const { return type_;}

    GroupNode* parent() const { return parent_; }
    int subGroupsCount() const { return children_.count(); }
    GroupNode* subGroup(int index) const;
    int indexOf(GroupNode* node) const { return children_.indexOf(node);}

    void setName(const QString& name);

    int myIndex() const;

    void addChildNode(const QVariantMap& map);
    GroupNode *findById(const int groupId);

    void removeChildNode(int index);

    int nodeCount() const;

    static GroupNode* createNodes(const QVariantList &list);

private:
    int id_;
    QString name_;
    GroupType type_;

    GroupNode* parent_;
    QList<GroupNode*> children_;

};

#endif // GROUPNODE_H
