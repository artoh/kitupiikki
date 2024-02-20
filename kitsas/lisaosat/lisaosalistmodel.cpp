#include "lisaosalistmodel.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/avattupilvi.h"
#include "qfont.h"

LisaosaListModel::LisaosaListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}


int LisaosaListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return lisaosat_.count();
}

QVariant LisaosaListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const Lisaosa& osa = lisaosat_.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return osa.nimi().teksti();
    case IdRooli:
        return osa.id();
    case NimiRooli:
        return osa.nimi().teksti();
    case AktiivinenRooli:
        return osa.active();
    case SystemRooli:
        return osa.system();
    case Qt::DecorationRole:
        if(osa.active())
            return QIcon(":/pic/palat.png");
        else
            return QIcon(":/pic/palat-harmaa.png");
    case Qt::FontRole:
        if( osa.active()) {
            QFont font;
            font.setBold(true);
            return font;
        } else {
            return QVariant();
        }
    default:
        return QVariant();
    }

}

void LisaosaListModel::load()
{
    if( !kp()->pilvi()) return;
    const QString bookId = kp()->pilvi()->pilvi().bookId();
    KpKysely* kysely = kp()->pilvi()->loginKysely("/v1/addons");
    kysely->lisaaAttribuutti("target", bookId);
    connect( kysely, &KpKysely::vastaus, this, &LisaosaListModel::dataSaapuu);
    kysely->kysy();
}

void LisaosaListModel::dataSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    beginResetModel();
    lisaosat_.clear();

    for(auto const& item : lista) {
        Lisaosa osa(item.toMap());
        lisaosat_.append(osa);
    }

    endResetModel();

}
