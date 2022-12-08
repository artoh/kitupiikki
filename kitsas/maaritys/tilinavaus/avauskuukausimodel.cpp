#include "avauskuukausimodel.h"

#include "db/kirjanpito.h"

AvausKuukausiModel::AvausKuukausiModel(QObject *parent, TilinavausModel::Erittely kohdennukset)
    : AvausEraKantaModel(parent),
      kohdennukset_{kohdennukset}
{    
}

QVariant AvausKuukausiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role==Qt::DisplayRole){
        switch (section) {
        case KUUKAUSI:
            return tr("Kuukausi");
        case SALDO:
            return tr("Saldo");
        case ERITTELY:
            return tr("Erittely");
        }
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

    return  kohdennukset_ ? 3 : 2;
}

QVariant AvausKuukausiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const AvausKuukausi& kuukausi = kuukaudet_.at(index.row());

    if( role == Qt::DisplayRole) {
        if( index.column() == KUUKAUSI) {
            return QString("%1 %2")
                    .arg( kp()->kaanna( QString("KK%1").arg(kuukausi.pvm().month()) )  )
                    .arg( kuukausi.pvm().year() );
        } else if(index.column() == SALDO) {
            return kuukausi.saldo().display();
        } else {
            const int erat = kuukausi.erat().count();
            if(erat)
                return kuukausi.erat().count();
            return QVariant();
        }
    } else if( role == Qt::EditRole && index.column() == SALDO) {
        return kuukausi.saldo().toString();
    } else if( role == Qt::DecorationRole && index.column() == ERITTELY) {
        if( kohdennukset_ == TilinavausModel::KOHDENNUKSET)
            return QIcon(":/pic/kohdennus.png");
        else if( kohdennukset_ == TilinavausModel::TASEERAT)
            return QIcon(":/pic/format-list-unordered.png");
    }

    // FIXME: Implement me!
    return QVariant();
}

bool AvausKuukausiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if( role == Qt::EditRole && index.column() == SALDO) {
        kuukaudet_[ index.row() ].asetaEuro( value.toString() );
        emit dataChanged(index, index, QVector<int>() << role);
    }
    return false;
}

Qt::ItemFlags AvausKuukausiModel::flags(const QModelIndex &index) const
{
    if( !index.isValid() || index.column() != SALDO)
        return Qt::ItemIsEnabled;
    else
        return Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void AvausKuukausiModel::lataa(QList<AvausEra> erat)
{
    beginResetModel();
    alustaKuukaudet();
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

QList<AvausEra> AvausKuukausiModel::kuukaudenErat(int kuukausi) const
{
    return kuukaudet_.at(kuukausi).erat();
}

void AvausKuukausiModel::asetaKuukaudenErat(const int kuukausi, const QList<AvausEra> erat)
{
    kuukaudet_[kuukausi].asetaErat(erat);
    emit dataChanged( createIndex(kuukausi, SALDO), createIndex(kuukausi, ERITTELY) );
}

QList<AvausEra> AvausKuukausiModel::erat() const
{
    QList<AvausEra> erat;
    for(const AvausKuukausi& kuukausi : qAsConst(kuukaudet_)) {
        for(const AvausEra& era : kuukausi.erat()) {
            if( era.saldo())
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
    kuukaudet_.clear();
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
    erat_.clear();
    for( auto era : erat) {
        era.asetaPvm(pvm());
        erat_.append(era);
    }
}
