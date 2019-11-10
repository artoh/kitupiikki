#include "asiakkaatmodel.h"
#include "db/kirjanpito.h"
#include <QSqlQuery>

#include <QDebug>

AsiakkaatModel::AsiakkaatModel(QObject *parent, KumppaniValinta valinta)
    : QAbstractTableModel(parent), valinta_(valinta)
{

}

int AsiakkaatModel::rowCount(const QModelIndex &/*parent*/) const
{
    return lista_.count();
}

int AsiakkaatModel::columnCount(const QModelIndex &/*parent*/) const
{
    if( valinta_ == REKISTERI)
        return 1;
    return 4;
}

QVariant AsiakkaatModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    AsiakasRivi rivi = rivit_.value(index.row());
    QVariantMap map = lista_.at(index.row()).toMap();

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column()) {
        case NIMI:
            return map.value("nimi");
        case YHTEENSA:

            if( map.value("summa").toDouble() > 1e-5 )
                return QString("%L1 €").arg( map.value("summa").toDouble() ,0,'f',2);
            break;
        case AVOINNA:
            if( map.value("avoin").toDouble() > 1e-5 )
                return QString("%L1 €").arg(  map.value("avoin").toDouble() ,0,'f',2);
            break;
        case ERAANTYNYT:
            if( map.value("eraantynyt").toDouble() > 1e-5 )
                return QString("%L1 €").arg( map.value("eraantynyt").toDouble() ,0,'f',2);
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column() == NIMI)
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
        else
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
    else if( role == Qt::TextColorRole && index.column() == ERAANTYNYT)
        return QColor(Qt::red);
    else if( role == IdRooli)
        return map.value("id");
    else if( role == NimiRooli)
        return map.value("nimi");
    return QVariant();
}

QVariant AsiakkaatModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter);
    else if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section) {
        case NIMI: return tr("Nimi");
        case YHTEENSA:
            return tr("Yhteensä");
        case AVOINNA:
            return tr("Avoinna");
        case ERAANTYNYT:
            return tr("Erääntynyt");
        }
    }
    return QVariant();
}


void AsiakkaatModel::paivita(int valinta)
{
    valinta_ = valinta;
    KpKysely *utelu = nullptr;

    if( valinta == TOIMITTAJAT)
        utelu = kpk("/toimittajat");
    else if(valinta == ASIAKKAAT)
        utelu = kpk("/asiakkaat");
    else
        utelu = kpk("/kumppanit");

    connect( utelu, &KpKysely::vastaus, this, &AsiakkaatModel::tietoSaapuu);
    utelu->kysy();

}

void AsiakkaatModel::tietoSaapuu(QVariant *var)
{
    beginResetModel();
    lista_ = var->toList();
    endResetModel();
}


