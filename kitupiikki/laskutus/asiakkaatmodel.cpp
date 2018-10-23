#include "asiakkaatmodel.h"
#include "db/kirjanpito.h"
#include <QSqlQuery>

#include <QDebug>

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
            if( rivi.yhteensa)
                return QString("%L1 €").arg(rivi.yhteensa / 100.0,0,'f',2);
            break;
        case AVOINNA:
            if( rivi.avoinna)
                return QString("%L1 €").arg(rivi.avoinna / 100.0,0,'f',2);
            break;
        case ERAANTYNYT:
            if( rivi.eraantynyt)
                return QString("%L1 €").arg(rivi.eraantynyt / 100.0,0,'f',2);
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
    else if( role == NimiRooli)
        return rivi.nimi;
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

        if( rivi.nimi.isEmpty())
            continue;

        // TODO: Summien laskeminen eri kyselyllä
        QString summakysely = QString("SELECT id, pvm, debetsnt, kreditsnt, erapvm, eraid FROM vienti "
                                      "WHERE asiakas=\"%1\" and iban is ").arg(rivi.nimi.replace("\"","\\\""));
        if( toimittajat_)
            summakysely.append("not ");
        summakysely.append("null");

        qlonglong summa=0;
        qlonglong eraantyneet=0;
        qlonglong avoimet = 0;

        QSqlQuery summaquery( summakysely );
        while( summaquery.next())
        {
           qlonglong sentit = toimittajat_ ? summaquery.value("kreditsnt").toLongLong() - summaquery.value("debetsnt").toLongLong()  :  summaquery.value("debetsnt").toLongLong() - summaquery.value("kreditsnt").toLongLong();
           TaseEra era( summaquery.value("eraid").toInt() );

            summa += sentit;
            avoimet += toimittajat_ ? 0 - era.saldoSnt : era.saldoSnt;
            QDate erapvm = summaquery.value("erapvm").toDate();
            if( erapvm.isValid() && erapvm < kp()->paivamaara())
                eraantyneet += toimittajat_ ? 0 - era.saldoSnt : era.saldoSnt;;
        }
        rivi.yhteensa = summa;
        rivi.avoinna = avoimet;
        rivi.eraantynyt = eraantyneet;

        rivit_.append(rivi);

    }
    endResetModel();
}


