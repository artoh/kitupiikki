#include "tiliotealvdelegaatti.h"

#include "tiliotealvcombo.h"

TilioteAlvDelegaatti::TilioteAlvDelegaatti(QObject *parent)
    : QItemDelegate{parent}
{

}

QWidget *TilioteAlvDelegaatti::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    TilioteAlvCombo* combo = new TilioteAlvCombo(parent);
    return combo;
}

void TilioteAlvDelegaatti::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TilioteAlvCombo* combo = qobject_cast<TilioteAlvCombo*>(editor);
    combo->aseta( (int) index.data(Qt::EditRole).toDouble() );

}

void TilioteAlvDelegaatti::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TilioteAlvCombo* combo = qobject_cast<TilioteAlvCombo*>(editor);
    model->setData(index, combo->prosentti());
}
