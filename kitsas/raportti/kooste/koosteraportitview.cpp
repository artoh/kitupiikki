#include "koosteraportitview.h"
#include "koosteraporttilistamodel.h"

#include "naytin/naytinikkuna.h"

#include <QHeaderView>

KoosteRaportitView::KoosteRaportitView()
{
    KoosteRaporttiListaModel* model = new KoosteRaporttiListaModel(this);
    setModel( model );

    setSelectionBehavior( SelectionBehavior::SelectRows);
    setSelectionMode( SelectionMode::SingleSelection);

    horizontalHeader()->setSectionResizeMode(KoosteRaporttiListaModel::LAATINUT, QHeaderView::Stretch);
    connect( this, &KoosteRaportitView::clicked, this, &KoosteRaportitView::naytaRaportti);
    connect( model, &KoosteRaporttiListaModel::modelReset, this, &KoosteRaportitView::resizeColumnsToContents);
}

void KoosteRaportitView::naytaRaportti(const QModelIndex &index)
{
    const int id = index.data(Qt::UserRole).toInt();
    if(id) {
        NaytinIkkuna::naytaLiite(id);
    }
}
