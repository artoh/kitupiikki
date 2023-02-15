#include "liitteetview.h"

#include "liitteetmodel.h"

LiitteetView::LiitteetView(QWidget* parent) :
    QListView(parent)
{

}

void LiitteetView::setLiitteetModel(LiitteetModel *model)
{
    model_ = model;
    setModel(model);

    connect( model, &LiitteetModel::valittuVaihtui, this, &LiitteetView::valitse);
    connect( selectionModel(), &QItemSelectionModel::currentChanged, this, &LiitteetView::valintaMuuttui );

    setDropIndicatorShown(true);

}

void LiitteetView::valitse(int indeksi)
{
    setCurrentIndex( model_->index(indeksi,0) );
}

void LiitteetView::valintaMuuttui(const QModelIndex &uusi)
{
    model_->nayta(uusi.row());
}
