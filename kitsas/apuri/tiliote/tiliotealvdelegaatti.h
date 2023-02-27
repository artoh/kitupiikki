#ifndef TILIOTEALVDELEGAATTI_H
#define TILIOTEALVDELEGAATTI_H

#include <QItemDelegate>

class TilioteAlvDelegaatti : public QItemDelegate
{
public:
    explicit TilioteAlvDelegaatti(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

};

#endif // TILIOTEALVDELEGAATTI_H
