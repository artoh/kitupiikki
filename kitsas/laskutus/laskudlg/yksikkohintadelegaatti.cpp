#include "yksikkohintadelegaatti.h"

#include "tools/kpyhedit.h"

YksikkoHintaDelegaatti::YksikkoHintaDelegaatti(QObject *parent) : QItemDelegate(parent)
{

}

QWidget *YksikkoHintaDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    KpYhEdit* edit = new KpYhEdit(parent);
    return edit;
}

void YksikkoHintaDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KpYhEdit* edit = qobject_cast<KpYhEdit*>(editor);
    edit->setValue(index.data(Qt::EditRole).toDouble());
}

void YksikkoHintaDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KpYhEdit* edit = qobject_cast<KpYhEdit*>(editor);
    model->setData(index, edit->value());
}
