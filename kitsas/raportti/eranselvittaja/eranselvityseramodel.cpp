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

    const SelvitysEra& era = erat_.at(index.row());

    if( role == Qt::DisplayRole) {
        switch( index.column()) {
        case PVM:
            return era.pvm().isValid() ? era.pvm().toString("dd.MM.yyyy") : "";
        case KUMPPANI:
            return era.nimi();
        case SELITE:
            if( era.id() == 0)
                return tr("Erittelemättömät");
            else
                return era.selite();
        case SALDO: {
            Euro saldo = era.saldo();
            if( QString::number(tili_.numero()).startsWith("1"))
                saldo = Euro::Zero - saldo;
            return saldo.display(true);
            }
        }
    } else if( role == Qt::TextAlignmentRole) {
        if( index.column() == SALDO) return Qt::AlignRight;
    } else if( role == Qt::UserRole) {
        return era.id();
    } else if( role == Qt::DecorationRole && index.column() == SELITE) {
        if( tili_.eritellaankoTase() && era.id() == 0) {
            return QIcon(":/pic/huomio.png");
        } else {
            return EraMap::kuvakeIdlla(era.id());
        }
    }

    return QVariant();
}

void EranSelvitysEraModel::load(const int tili, const QDate& date)
{
    tili_ = kp()->tilit()->tiliNumerolla(tili);
    saldopvm_ = date;

    refresh();
}

void EranSelvitysEraModel::refresh()
{
    KpKysely* kysely = kpk("/erat/selvittely");
    kysely->lisaaAttribuutti("tili", tili_.numero());
    kysely->lisaaAttribuutti("pvm", saldopvm_);
    if( nollatut_)
        kysely->lisaaAttribuutti("nollat", "true");
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitysEraModel::eratSaapuu);
    kysely->kysy();
}

void EranSelvitysEraModel::naytaNollatut(bool nollatut)
{
    nollatut_ = nollatut;
    refresh();
}

void EranSelvitysEraModel::eratSaapuu(QVariant *data)
{
    beginResetModel();
    erat_.clear();
    for(const auto& item : data->toList()) {
        QVariantMap map = item.toMap();
        erat_.append( SelvitysEra(map));
    }
    endResetModel();
}

EranSelvitysEraModel::SelvitysEra::SelvitysEra()
{

}

EranSelvitysEraModel::SelvitysEra::SelvitysEra(const QVariantMap &map)
{
    id_ = map.value("id").toInt();
    pvm_ = map.value("pvm").toDate();
    saldo_ = Euro::fromVariant(map.value("saldo"));
    nimi_ = map.value("nimi").toString();
    selite_ = map.value("selite").toString();
}
