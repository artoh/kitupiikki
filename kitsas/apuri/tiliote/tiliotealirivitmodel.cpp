#include "tiliotealirivitmodel.h"
#include "db/kitsasinterface.h"
#include "db/tilimodel.h"
#include "db/verotyyppimodel.h"

TilioteAliRivitModel::TilioteAliRivitModel(KitsasInterface *interface, TilioteKirjausRivi *kirjausRivi, QObject *parent)
    : QAbstractTableModel(parent),
    kirjausRivi_(kirjausRivi),
    kitsasInterface_(interface)
{
}

QVariant TilioteAliRivitModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case TILI:
            return tr("Tili");
        case ALV:
            return tr("Alv");
        case EURO:
            return "€";
        }
    }
    return QVariant();
}

int TilioteAliRivitModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return kirjausRivi_->riveja();
}

int TilioteAliRivitModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant TilioteAliRivitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    const TilioteAliRivi& alirivi = kirjausRivi_->rivi(index.row());

    if( role == Qt::DisplayRole) {
        if( index.column() == TILI ) {
            Tili* tili = kitsasInterface_->tilit()->tili( alirivi.tilinumero() );
            if( tili )
                return tili->nimiNumero();
            return QVariant();
        } else if( index.column() == ALV) {
            double alv = alirivi.alvprosentti();
            int alvtyyppi = alirivi.alvkoodi();
            if( alv < 1e-3 || kitsasInterface_->alvTyypit()->nollaTyyppi(alvtyyppi))
                return QVariant();
            else
                return QString("%1 %").arg((int) alv);
        } else if( index.column() == EURO) {
            if( alirivi.naytaBrutto())
                return alirivi.brutto().display();
            else
                return alirivi.netto().display();
        }
    }
    else if( role == Qt::DecorationRole && index.column() == ALV) {
        return kitsasInterface_->alvTyypit()->kuvakeKoodilla( alirivi.alvkoodi() );
    }

    return QVariant();
}

TilioteAliRivi TilioteAliRivitModel::rivi(int indeksi) const
{
    return kirjausRivi_->rivi(indeksi);
}

void TilioteAliRivitModel::tyhjenna()
{
    beginResetModel();
    kirjausRivi_->tyhjenna();
    endResetModel();
}

void TilioteAliRivitModel::lataa(TilioteKirjausRivi *rivi)
{
    beginResetModel();
    kirjausRivi_ = rivi;
    endResetModel();
}

void TilioteAliRivitModel::korvaa(int rivi, const TilioteAliRivi &uusi)
{
    kirjausRivi_->asetaRivi(rivi, uusi);
    emit dataChanged(index(rivi,TILI), index(rivi,TILI));
}

void TilioteAliRivitModel::poista(int indeksi)
{
    beginRemoveRows(QModelIndex(), indeksi, indeksi);
    kirjausRivi_->poistaRivi(indeksi);
    endRemoveRows();
}

void TilioteAliRivitModel::uusiRivi()
{
    beginInsertRows(QModelIndex(), kirjausRivi_->riveja(), kirjausRivi_->riveja());
    kirjausRivi_->lisaaRivi();
    endInsertRows();
}
