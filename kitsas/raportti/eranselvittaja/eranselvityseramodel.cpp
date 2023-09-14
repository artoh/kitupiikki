#include "eranselvityseramodel.h"
#include "db/kirjanpito.h"

EranSelvitysEraModel::EranSelvitysEraModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant EranSelvitysEraModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch( section ) {
        case PVM: return tr("Pvm");
        case KUMPPANI: return tr("Asiakas/Toimittaja");
        case SELITE: return tr("Selite");
        case SALDO: return tr("Saldo");
        }
    }
    return QVariant();
}

int EranSelvitysEraModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return erat_.count();
}

int EranSelvitysEraModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 4;
}

QVariant EranSelvitysEraModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const EraMap& era = erat_.at(index.row());

    if( role == Qt::DisplayRole) {
        switch( index.column()) {
        case PVM:
            return era.pvm().isValid() ? era.pvm().toString("dd.MM.yyyy") : "";
        case KUMPPANI:
            return era.kumppaniNimi();
        case SELITE:
            if( era.id() == 0) return tr("Erittelemättömät");
            else if( era.eratyyppi() == EraMap::Asiakas) return era.asiakasNimi();
            else if( era.eratyyppi() == EraMap::Huoneisto) return era.huoneistoNimi();
            else return era.nimi();
        case SALDO:
            return era.saldo().display(true);
        }
    } else if( role == Qt::TextAlignmentRole) {
        if( index.column() == SALDO) return Qt::AlignRight;
    } else if( role == Qt::UserRole) {
        return era.id();
    } else if( role == Qt::DecorationRole && index.column() == SELITE) {
        if( tili_.eritellaankoTase() && era.eratyyppi() == EraMap::EiEraa) {
            return QIcon(":/pic/huomio.png");
        } else {
            return era.tyyppiKuvake();
        }
    }

    return QVariant();
}

void EranSelvitysEraModel::load(const int tili, const QDate& date)
{
    KpKysely* kysely = kpk("/erat");
    kysely->lisaaAttribuutti("tili", tili);
    kysely->lisaaAttribuutti("selvitys");
    kysely->lisaaAttribuutti("pvm", date);
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitysEraModel::eratSaapuu);
    kysely->kysy();

    tili_ = kp()->tilit()->tiliNumerolla(tili);
}

void EranSelvitysEraModel::eratSaapuu(QVariant *data)
{
    beginResetModel();
    erat_.clear();
    for(const auto& item : data->toList()) {
        QVariantMap map = item.toMap();
        erat_.append( EraMap(map));
    }
    endResetModel();
}
