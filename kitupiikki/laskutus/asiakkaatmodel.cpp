#include "asiakkaatmodel.h"

#include <QSqlQuery>

AsiakkaatModel::AsiakkaatModel(QObject *parent, bool toimittajat)
    : QAbstractTableModel(parent), toimittajat_(toimittajat)
{

}

int AsiakkaatModel::rowCount(const QModelIndex &/*parent*/) const
{
    return rivit_.count();
}

int AsiakkaatModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;
}

QVariant AsiakkaatModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();

    AsiakasRivi rivi = rivit_.value(index.row());
    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column()) {
        case NIMI:
            return rivi.nimi;
        case YHTEENSA:
            return QString("%L1 €").arg(rivi.yhteensa / 100.0,0,'f',2);
        case AVOINNA:
            return QString("%L1 €").arg(rivi.avoinna / 100.0,0,'f',2);
        case ERAANTYNYT:
            return QString("%L1 €").arg(rivi.eraantynyt / 100.0,0,'f',2);
        }
    }
    else if( role == Qt::TextAlignmentRole)
    {
        if( index.column() == NIMI)
            return QVariant( Qt::AlignLeft | Qt::AlignVCenter);
        else
            return QVariant(Qt::AlignRight | Qt::AlignRight);
    }
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
            return toimittajat_ ? tr("Ostot yhteensä") : tr("Myynti yhteensä");
        case AVOINNA:
            return tr("Avoinna");
        case ERAANTYNYT:
            return tr("Erääntynyt");
        }
    }
    return QVariant();
}

void AsiakkaatModel::paivita(bool toimittajat)
{
    toimittajat_ = toimittajat;

    QString kysely = "select distinct asiakas from vienti where iban is ";

    if( toimittajat_ )
        kysely.append("not");

    kysely.append(" null order by asiakas");

    beginResetModel();
    rivit_.clear();
    QSqlQuery query( kysely );

    while( query.next())
    {
        AsiakasRivi rivi;
        rivi.nimi = query.value(0).toString();

        // TODO: Summien laskeminen eri kyselyllä

        rivit_.append(rivi);
    }
    endResetModel();
}


