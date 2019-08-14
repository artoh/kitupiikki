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
    return lista_.count();
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
    QVariantMap map = lista_.at(index.row()).toMap();

    if( role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch (index.column()) {
        case NIMI:
            return map.value("nimi");
        case YHTEENSA:
            qDebug() << map.value("summa");

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

void AsiakkaatModel::paivita(bool toimittajat)
{
    toimittajat_ = toimittajat;
    KpKysely *utelu = nullptr;

    if( toimittajat )
        utelu = kpk("/toimittajat");
    else
        utelu = kpk("/asiakkaat");
    connect( utelu, &KpKysely::vastaus, this, &AsiakkaatModel::tietoSaapuu);
    utelu->kysy();
    return;



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

        // Summien laskeminen eri kyselyllä
        QString summakysely = QString("SELECT id, pvm, debetsnt, kreditsnt, erapvm, eraid, tili FROM vienti "
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
           summa += sentit;

           int eraId = summaquery.value("eraid").toInt();
           TaseEra era( eraId );

           qlonglong avoinsnt = toimittajat_ ? 0 - era.saldoSnt : era.saldoSnt;
           if( avoinsnt > sentit)
               avoinsnt = sentit;

            if( !toimittajat_ ||  kp()->tilit()->tiliIdllaVanha( summaquery.value("tili").toInt() ).onko(TiliLaji::OSTOVELKA))
            {
                avoimet += avoinsnt;
                QDate erapvm = summaquery.value("erapvm").toDate();
                if( erapvm.isValid() && erapvm < kp()->paivamaara())
                    eraantyneet += avoinsnt;
            }
        }
        rivi.yhteensa = summa;
        rivi.avoinna = avoimet;
        rivi.eraantynyt = eraantyneet;

        rivit_.append(rivi);

    }
    endResetModel();
}

void AsiakkaatModel::tietoSaapuu(QVariant *var)
{
    beginResetModel();
    lista_ = var->toList();
    endResetModel();
}


