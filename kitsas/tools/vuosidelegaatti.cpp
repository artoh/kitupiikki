#include "vuosidelegaatti.h"

#include <QSpinBox>

VuosiDelegaatti::VuosiDelegaatti(QObject *parent) : QItemDelegate(parent)
{

}

QWidget *VuosiDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    QSpinBox *edit = new QSpinBox(parent);
    edit->setRange(0, 120);
    edit->setSuffix(tr(" vuotta"));
    return edit;
}

void VuosiDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QSpinBox* edit = qobject_cast<QSpinBox*>(editor);
    edit->setValue( index.data(Qt::EditRole).toInt() );
}

void VuosiDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox* edit = qobject_cast<QSpinBox*>(editor);
    model->setData(index, edit->value());
}
