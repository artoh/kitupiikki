#include "pankitmodel.h"

#include <QIcon>
#include "pilvi/pilvikysely.h"
#include "db/kirjanpito.h"

namespace Tilitieto {

PankitModel::PankitModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PankitModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return pankit_.count();
}

QVariant PankitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();    

    const Pankki* pankki = pankit_.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return pankki->nimi();
    case IdRooli:
        return pankki->id();
    case BicRooli:
        return pankki->bic();
    case Qt::DecorationRole:
        return pankki->icon();
    default:
        return QVariant();
    }
}


int PankitModel::indeksiBicilla(const QString &bic) const
{
    for(int i=0; i < rowCount(); i++) {
        if( bic == pankit_.at(i)->bic()) {
            return i;
        }
    }
    return -1;
}

Pankki* PankitModel::pankki(int id)
{
    for(Pankki* p : pankit_) {
        if( p->id() == id) {
            return p;
        }
    }
    return nullptr;
}

void PankitModel::haePankit()
{
    const QString url = kp()->pilvi()->kbcOsoite() + "/institutions";
    PilviKysely* pk = new PilviKysely(kp()->pilvi(), KpKysely::GET, url);
    connect( pk, &PilviKysely::vastaus, this, &PankitModel::pankitSaapuu);
    pk->kysy();

}

void PankitModel::pankitSaapuu(QVariant *data)
{
    pankit_.clear();
    QVariantList lista = data->toList();
    for(const auto& item : lista) {
        QVariantMap map = item.toMap();
        pankit_.append(new Pankki(map, this));
    }
}


} // namespace Tilitieto
