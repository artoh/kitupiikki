#include "avauskuukausimodel.h"

#include "db/kirjanpito.h"
#include "db/tilikausimodel.h"

AvausKuukausiModel::AvausKuukausiModel(QObject *parent)
    : AvausEraKantaModel(parent)
{    
}

QVariant AvausKuukausiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role==Qt::DisplayRole){
        return section ? tr("Saldo") : tr("Kuukausi");
    }
    return QVariant();
}

int AvausKuukausiModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return kuukaudet_.count();
}

int AvausKuukausiModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant AvausKuukausiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const AvausKuukausi& kuukausi = kuukaudet_.at(index.row());

    if( role == Qt::DisplayRole) {
        if( index.column() == KUUKAUSI) {
            return kuukausi.pvm().toString("MMMM YYYY");
        } else {
            return kuukausi.saldo().display();
        }
    }

    // FIXME: Implement me!
    return QVariant();
}

void AvausKuukausiModel::lataa(QList<AvausEra> erat)
{
    beginResetModel();
    for(const AvausEra& era : qAsConst(erat)) {
        for(int i=0; i < kuukaudet_.count(); i++) {
            if( kuukaudet_.at(i).pvm() == era.pvm()) {
                kuukaudet_[i].lisaaEra(era);
                break;
            }
        }
    }
    endResetModel();
}

QList<AvausEra> AvausKuukausiModel::erat() const
{
    QList<AvausEra> erat;
    for(const AvausKuukausi& kuukausi : qAsConst(kuukaudet_)) {
        for(const AvausEra& era : kuukausi.erat()) {
            erat.append(era);
        }
    }
    return erat;
}

Euro AvausKuukausiModel::summa() const
{
    Euro s;
    for(const AvausKuukausi& kuukausi : qAsConst(kuukaudet_)) {
        s += kuukausi.saldo();
    }
    return s;
}

void AvausKuukausiModel::alustaKuukaudet()
{
    const QDate avausPvm = kp()->asetukset()->pvm(AsetusModel::TilinAvausPvm);
    const Tilikausi kausi = kp()->tilikausiPaivalle(avausPvm);

    QDate pvm = kausi.alkaa();
    while( pvm < avausPvm) {
        kuukaudet_.append( AvausKuukausi( QDate( pvm.year(), pvm.month(), pvm.daysInMonth() ) ) );
        pvm = pvm.addMonths(1);
    }

}

AvausKuukausiModel::AvausKuukausi::AvausKuukausi(const QDate &pvm) :
    pvm_{pvm}
{

}

Euro AvausKuukausiModel::AvausKuukausi::saldo() const
{
    Euro summa;
    for(const AvausEra& era : qAsConst(erat_)) {
        summa += era.saldo();
    }
    return summa;
}

void AvausKuukausiModel::AvausKuukausi::lisaaEra(const AvausEra &era)
{
    erat_.append(era);
}

void AvausKuukausiModel::AvausKuukausi::asetaEuro(const Euro &euro)
{
    erat_.clear();
    erat_.append(AvausEra(euro, pvm_));
}

void AvausKuukausiModel::AvausKuukausi::asetaErat(const QList<AvausEra> erat)
{
    erat_ = erat;
}
