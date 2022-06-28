#ifndef YKSIKKOHINTADELEGAATTI_H
#define YKSIKKOHINTADELEGAATTI_H

#include <QItemDelegate>

class YksikkoHintaDelegaatti : public QItemDelegate
{
public:
    YksikkoHintaDelegaatti(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

};

#endif // YKSIKKOHINTADELEGAATTI_H
