#include "eranselvityseramodel.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "tools/eranvalintamodel.h"

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
        case KAUSI: return tr("Kausi");
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

    return 5;
}

QVariant EranSelvitysEraModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const SelvitysEra& era = erat_.at(index.row());

    if( role == Qt::DisplayRole | role == Qt::EditRole) {
        switch( index.column()) {
        case PVM:
            return role == Qt::DisplayRole ?
                (era.pvm().isValid() ? era.pvm().toString("dd.MM.yyyy") : "") :
                (era.pvm().isValid() ? era.pvm().toString("yyyy-MM-dd") : "") ;
        case KUMPPANI:
            return era.nimi();
        case SELITE:
            if( era.id() == 0)
                return tr("Erittelemättömät");
            else
                return era.selite();
        case KAUSI:
            return era.kausi().display(true);
        case SALDO:
            return era.saldo().display(true);

        }
    } else if( role == Qt::TextAlignmentRole) {
        if( index.column() == SALDO || index.column() == KAUSI) return (int(Qt::AlignRight) | int(Qt::AlignVCenter));
    } else if( role == IdRooli || role == EranValintaModel::IdRooli) {
        return era.id();
    } else if( role == EraMapRooli || role == EranValintaModel::MapRooli) {
        return era.eraMap();
    } else if( role == EranValintaModel::PvmRooli) {
        return era.pvm();
    } else if( role == EranValintaModel::TekstiRooli) {
        return era.selite();
    } else if( role == Qt::DecorationRole && index.column() == SELITE) {
        if( !tili_.onkoValidi() )
            return QIcon(":/pic/punainen.png");
        else if( era.invalid()) {
            return QIcon(":/pic/oranssi.png");
        } else if( era.id() == 0) {
            return tili_.eritellaankoTase() ? QIcon(":/pic/keltainen.png") : QIcon(":/pic/harmaa.png");
        } else {
            return EraMap::kuvakeIdlla(era.id());
        }
    }

    return QVariant();
}

Qt::ItemFlags EranSelvitysEraModel::flags(const QModelIndex &index) const
{
    const int eraid = index.data(Qt::UserRole).toInt();

    if( index.column() == SELITE && eraid > 0 && kp()->yhteysModel()->onkoOikeutta(YhteysModel::TOSITE_MUOKKAUS)) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool EranSelvitysEraModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::EditRole) {
        if( index.column() == SELITE) {
            SelvitysEra era = erat_.at(index.row());
            era.setSelite(value.toString());
            erat_[index.row()] = era;

            KpKysely* kysely = kpk(QString("/viennit/%1").arg(era.id()), KpKysely::PATCH);
            QVariantMap data;
            data.insert("selite", value.toString());
            kysely->kysy(data);

            emit dataChanged(index, index);
            return true;
        }
    }
    return false;
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
    const bool vastaavaa = tili_.onko(TiliLaji::VASTAAVAA);
    for(const auto& item : data->toList()) {
        QVariantMap map = item.toMap();
        erat_.append( SelvitysEra(map, vastaavaa));
    }
    endResetModel();
}

EranSelvitysEraModel::SelvitysEra::SelvitysEra()
{

}

EranSelvitysEraModel::SelvitysEra::SelvitysEra(const QVariantMap &map, bool vastaavaa)
{
    id_ = map.value("id").toInt();
    pvm_ = map.value("pvm").toDate();

    Euro saldo = Euro::fromVariant(map.value("saldo"));
    Euro kausi = Euro::fromVariant(map.value("kausi"));

    saldo_ = vastaavaa ? Euro::Zero - saldo : saldo;
    kausi_ = vastaavaa ? Euro::Zero - kausi : kausi;


    nimi_ = map.value("nimi").toString();
    kumppaniId_=map.value("kumppaniid").toInt();
    selite_ = map.value("selite").toString();
    tunniste_ = map.value("tunniste").toInt();
    sarja_ = map.value("sarja").toString();
    eraok_ = map.value("eraok").toBool() || map.value("id").isNull() || id_ < 0;
}

bool EranSelvitysEraModel::SelvitysEra::invalid() const
{
    if( !eraok_ || (id_ > 0 && !tunniste() ) )
        return true;
    return false;
}

void EranSelvitysEraModel::SelvitysEra::setSelite(const QString &selite)
{
    selite_ = selite;
}

QVariantMap EranSelvitysEraModel::SelvitysEra::eraMap() const
{
    QVariantMap map;
    map.insert("id", id_);
    map.insert("selite", selite_);
    map.insert("pvm", pvm_);
    map.insert("sarja", sarja_);
    map.insert("tunniste", tunniste_);
    map.insert("saldo", saldo_.toString());
    if(kumppaniId_) {
        QVariantMap kmap;
        kmap.insert("nimi", nimi_);
        kmap.insert("id", kumppaniId_);
        map.insert("kumppani", kmap);
    }

    if( id_ % 10 == EraMap::EraTyyppi::Asiakas) {
        QVariantMap amap;
        amap.insert("nimi", selite_);
        amap.insert("id", id_ / -10);
        map.insert("asiakas", amap);
    } else if( id_ % 10 == EraMap::EraTyyppi::Huoneisto) {
        QVariantMap hmap;
        hmap.insert("nimi", selite_);
        hmap.insert("id", id_ / -10);
        map.insert("huoneisto", hmap);
    }
    return map;
}
