#include "eranselvitystilimodel.h"
#include "db/kirjanpito.h"

EranSelvitysTiliModel::EranSelvitysTiliModel(QDate startDate, QDate date, QObject *parent)
    : QAbstractTableModel(parent),
    startDate_{ startDate},
    date_{ date }
{
    refresh();
}

QVariant EranSelvitysTiliModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case TILI:
            return tr("Tili");
        case KAUSI:
            return tr("Kausi");
        case SALDO:
            return tr("Saldo");
        }
    }
    return QVariant();
}

int EranSelvitysTiliModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return data_.count();
}

int EranSelvitysTiliModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant EranSelvitysTiliModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole) {
        const EranSelvitysTili& rData = data_.at(index.row());
        if( index.column() == TILI) {
            Tili* tili = kp()->tilit()->tili(rData.tili());
            if(tili)
                return tili->nimiNumero();
            else
                return tr("Tiliöimättä");
        } else if( index.column() == KAUSI) {
            return rData.kausi().display(true);
        } else if( index.column() == SALDO) {
            return rData.saldo().display(true);
        }
    } else if( role == Qt::UserRole) {
        return data_.at(index.row()).tili();
    } else if( role == Qt::TextAlignmentRole && (index.column() == SALDO || index.column() == KAUSI)) {
        return (int(Qt::AlignRight) | int(Qt::AlignVCenter));
    } else if( role == Qt::DecorationRole && index.column() == TILI) {
        if( data_.at(index.row()).tili() == 0)
            return QIcon(":/pic/punainen.png");
        else {
            const EranSelvitysTili& rData = data_.at(index.row());
            Tili* tili = kp()->tilit()->tili(rData.tili());
            if( !tili ) return QIcon(":/pic/varoitus.png");
            if(  tili->eritellaankoTase()) {
                return rData.erittelemattomia() ?
                    QIcon(":/pic/keltainen.png") :
                    QIcon(":/pic/kaytossa.png");
            } else {
                return QIcon(":pic/harmaa.png");
            }
        }
    }

    return QVariant();
}

void EranSelvitysTiliModel::refresh()
{
    KpKysely* kysely = kpk("/erat/selvittely");
    kysely->lisaaAttribuutti("pvm", date_);
    kysely->lisaaAttribuutti("alkuPvm", startDate_);
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitysTiliModel::saldotSaapuu);
    kysely->kysy();
}

void EranSelvitysTiliModel::saldotSaapuu(QVariant *data)
{
    QVariantList list = data->toList();
    beginResetModel();
    data_.clear();

    for(const auto& item : list) {
        data_.append( EranSelvitysTili(item.toMap()));
    }

    endResetModel();
}

EranSelvitysTiliModel::EranSelvitysTili::EranSelvitysTili()
{

}

EranSelvitysTiliModel::EranSelvitysTili::EranSelvitysTili(const QVariantMap &map)
{
    tili_ = map.value("tili").toInt();

    Euro saldo = Euro::fromVariant(map.value("saldo"));
    Euro kausi = Euro::fromVariant(map.value("kausi"));
    bool vastaavaa = map.value("tili").toString().startsWith("1");

    saldo_ = vastaavaa ? Euro::Zero - saldo : saldo;
    kausi_ = vastaavaa ? Euro::Zero - kausi : kausi;
    erittelemattomia_ = map.value("eraton").toInt();
}
