#include "pankkilokimodel.h"
#include "laskutus/iban.h"

#include <QDateTime>
#include <QColor>

namespace Tilitieto {

PankkiLokiModel::PankkiLokiModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant PankkiLokiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case AIKA: return tr("Aika");
            case TILI: return tr("Tili");
            case STATUS: return tr("Tila");
        }
    }
    return QVariant();
}

int PankkiLokiModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return loki_.count();
}

int PankkiLokiModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant PankkiLokiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    const LokiRivi& rivi = loki_.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    {
        switch (index.column()) {
        case AIKA:
            return rivi.aika();
        case TILI:
            return rivi.iban();
        case STATUS:
            return statusTeksti(rivi.status());
        }
    }
    case Qt::ForegroundRole: {
        if( rivi.status() == "OK")
            return QVariant(QColor(Qt::darkGreen));
        else if( rivi.status() == "TE")
            return QVariant(QColor(Qt::darkGray));
        else
            return QVariant(QColor(Qt::darkRed));
    }
    default:
        return QVariant();
    }
}

void PankkiLokiModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    for(QVariant item : lista) {
        loki_.append(item.toMap());
    }
    endResetModel();
}

QString PankkiLokiModel::statusTeksti(const QString &status)
{
    if( status == "OK") return tr("Onnistui");
    else if( status == "AM") return tr("Kirjanpidosta ei löytynyt tiliä IBAN-numerolla");
    else if( status == "TE") return tr("Ei tilitapahtumia");
    else if( status == "CE") return tr("Valtuutus vanhentunut");
    else if( status == "BB") return tr("Tiliote jo kirjanpidossa");
    else if( status == "ER") return tr("Epäonnistui virhetilanteen takia");
    else return tr("Virhetilanne %1").arg(status);

}

PankkiLokiModel::LokiRivi::LokiRivi(const QVariantMap &map)
{
    aika_ = map.value("time").toDateTime().toLocalTime().toString("dd.MM.yyyy hh.mm");
    status_ = map.value("status").toString();
    Iban iban(map.value("account").toMap().value("iban").toString());
    iban_ = iban.valeilla();
}

} // namespace Tilitieto
