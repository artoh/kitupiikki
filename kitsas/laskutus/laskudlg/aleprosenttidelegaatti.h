#ifndef ALEPROSENTTIDELEGAATTI_H
#define ALEPROSENTTIDELEGAATTI_H

#include <QItemDelegate>

class AleProsenttiDelegaatti : public QItemDelegate
{
public:
    explicit AleProsenttiDelegaatti(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

};

#endif // ALEPROSENTTIDELEGAATTI_H
