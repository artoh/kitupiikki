#include "aleprosenttidelegaatti.h"
#include <QDoubleSpinBox>

AleProsenttiDelegaatti::AleProsenttiDelegaatti(QObject *parent) : QItemDelegate(parent)
{

}

QWidget *AleProsenttiDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem & /* option */, const QModelIndex & /* index */) const
{
    QDoubleSpinBox* spin = new QDoubleSpinBox(parent);
    spin->setRange(0, 100);
    spin->setSuffix(" %");
    return spin;
}

void AleProsenttiDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(editor);
    double value = index.data(Qt::EditRole).toDouble();
    spin->setValue(value);
}

void AleProsenttiDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(editor);
    double value = spin->value();
    model->setData(index, value, Qt::EditRole);
}


