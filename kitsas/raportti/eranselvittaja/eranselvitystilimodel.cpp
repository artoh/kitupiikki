#include "eranselvitystilimodel.h"
#include "db/kirjanpito.h"

EranSelvitysTiliModel::EranSelvitysTiliModel(QDate date, QObject *parent)
    : QAbstractTableModel(parent),
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

    return 2;
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
        } else if( index.column() == SALDO) {
            return rData.saldo().display(true);
        }
    } else if( role == Qt::UserRole) {
        return data_.at(index.row()).tili();
    } else if( role == Qt::TextAlignmentRole && index.column() == SALDO) {
        return Qt::AlignRight;
    } else if( role == Qt::DecorationRole && index.column() == TILI) {
        if( data_.at(index.row()).tili() == 0)
            return QIcon(":/pic/punainen.png");
    }

    return QVariant();
}

void EranSelvitysTiliModel::refresh()
{
    KpKysely* kysely = kpk("/saldot");
    kysely->lisaaAttribuutti("pvm", date_);
    kysely->lisaaAttribuutti("tase");
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitysTiliModel::saldotSaapuu);
    kysely->kysy();
}

void EranSelvitysTiliModel::saldotSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    beginResetModel();
    data_.clear();
    QMapIterator<QString,QVariant> iter(map);
    while(iter.hasNext()) {
        iter.next();
        data_.append( EranSelvitysTili(iter.key().toInt(), iter.value().toString()) );
    }

    endResetModel();
}

EranSelvitysTiliModel::EranSelvitysTili::EranSelvitysTili()
{

}

EranSelvitysTiliModel::EranSelvitysTili::EranSelvitysTili(int tilinumero, Euro saldo) :
    tili_{tilinumero}, saldo_{ saldo }
{

}
