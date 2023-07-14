#include "koosteraporttilistamodel.h"

#include <QDateTime>
#include "db/kirjanpito.h"

KoosteRaporttiListaModel::KoosteRaporttiListaModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    load();
}

QVariant KoosteRaporttiListaModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case AJALTA: return tr("Ajalta");
        case LAATINUT: return tr("Laatinut");
        case LAADITTU: return tr("Lähetetty");
        }
    }
    return QVariant();
}

int KoosteRaporttiListaModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return data_.count();
}

int KoosteRaporttiListaModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant KoosteRaporttiListaModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const KoosteRaportti& item = data_.at(index.row());
    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case AJALTA:
            return item.ajalta();
        case LAATINUT:
            return item.luonut();
        case LAADITTU:
            return item.luotu();
        }
    }
    else if( role == Qt::UserRole) {
        return item.id();
    }

    return QVariant();
}

void KoosteRaporttiListaModel::load()
{
    KpKysely* kysely = kpk("/raportti/kooste");
    connect( kysely, &KpKysely::vastaus, this, &KoosteRaporttiListaModel::dataSaapuu);
    kysely->kysy();
}

void KoosteRaporttiListaModel::dataSaapuu(const QVariant *data)
{
    QVariantList lista = data->toList();
    beginResetModel();
    data_.clear();
    for(const auto& item: lista) {
        data_.append( KoosteRaportti(item.toMap()) );
    }
    endResetModel();
}

KoosteRaporttiListaModel::KoosteRaportti::KoosteRaportti()
{

}

KoosteRaporttiListaModel::KoosteRaportti::KoosteRaportti(const QVariantMap &data)
{
    luotu_ = data.value("created").toDateTime().toString("dd.MM.yyyy HH.mm");
    ajalta_ = data.value("periodStart").toDate().toString("dd.MM.yyyy") + " - " + data.value("periodEnd").toDateTime().toString("dd.MM.yyyy");
    luonut_ = data.value("creatorName").toString();
    id_ = data.value("id").toInt();
}
