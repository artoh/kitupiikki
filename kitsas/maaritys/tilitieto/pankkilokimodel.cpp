#include "pankkilokimodel.h"
#include "laskutus/iban.h"

#include <QDateTime>
#include <QColor>
#include <QIcon>

#include "db/kirjanpito.h"

namespace Tilitieto {

PankkiLokiModel::PankkiLokiModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant PankkiLokiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case AIKA: return tr("Noudettu");
            case AJALTA: return tr("Ajalta");
            case TILI: return tr("Tili");
            case IBAN: return tr("IBAN");
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

    return 5;
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
        case AJALTA: {
            if( !rivi.mista().isValid()) {
                return QVariant();
            } else if( rivi.mista() == rivi.mihin()) {
                return rivi.mista().toString("dd.MM.yyyy");
            } else {
                return QString("%1 - %2").
                        arg( rivi.mista().toString("dd.MM.yyyy"),
                             rivi.mihin().toString("dd.MM.yyyy"));
            }
        }
        case TILI:{
            Tili tili = kp()->tilit()->tiliIbanilla(rivi.iban());
            if( tili.onkoValidi() ) {
                return tili.nimiNumero();
            } else {
                return QVariant();
            }
        }
        case IBAN: {
            return Iban(rivi.iban()).valeilla();
        }
        case STATUS:
            return statusTeksti(rivi.status());
        }
        return QVariant();
    }
    case Qt::ForegroundRole: {
        if( rivi.status() == "OK")
            return QVariant(QColor(Qt::darkGreen));
        else if( rivi.status() == "TE" ||
                 rivi.status() == "BB")
            return QVariant(QColor(Qt::black));
        else
            return QVariant(QColor(Qt::darkRed));
    }
    case DocumentIdRole:
        return rivi.docId();
    case Qt::DecorationRole:
    {
        if( index.column() == AIKA ) {
            return rivi.user() == 0?
                        QIcon(":/pic/chardevice.png")
                      : QIcon(":/pic/mies.png");
        }
        if( index.column() == STATUS) {
            if( rivi.status() == "OK") {
                return QIcon(":/pic/ok.png");
            } else if( rivi.status() == "TE" ||
                       rivi.status() == "BB") {
                return QIcon(":/pic/tyhja.png");
            } else {
                return QIcon(":/pic/peru.png");
            }
        }
    }
    default:
        return QVariant();
    }
}

void PankkiLokiModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    loki_.clear();
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
    else if( status == "CE" || status == "AE") return tr("Valtuutus vanhentunut");
    else if( status == "BB") return tr("Tiliote jo kirjanpidossa");
    else if( status == "ER") return tr("Epäonnistui virhetilanteen takia");
    else if( status == "RL") return tr("Päivittäinen tilitietojen hakukiintiö ylitetty");
    else if( status == "UA") return tr("Pankin palvelu ei käytössä");
    else if( status == "EC") return tr("Yhteysvirhe");
    else return tr("Virhetilanne %1").arg(status);

}

PankkiLokiModel::LokiRivi::LokiRivi(const QVariantMap &map)
{
    aika_ = map.value("time").toDateTime().toLocalTime().toString("dd.MM.yyyy hh.mm");
    status_ = map.value("status").toString();
    iban_ = map.value("account").toMap().value("iban").toString();
    docId_ = map.value("document").toInt();
    mista_ = map.value("datefrom").toDate();
    mihin_ = map.value("dateto").toDate();
    user_ = map.value("user").toInt();

}

} // namespace Tilitieto
