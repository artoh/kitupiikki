#include "extralogmodel.h"

#include "kieli/monikielinen.h"

#include <QDateTime>
#include <QIcon>

ExtraLogModel::ExtraLogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant ExtraLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case AIKA: return tr("Aika");
            case INFO: return tr("Tapahtuma");
        }
    }
    return QVariant();
}

int ExtraLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return loki_.count();
}

int ExtraLogModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant ExtraLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const EkstraLokiRivi& rivi = loki_.at(index.row());
    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case AIKA:
            return rivi.aika();
        case INFO:
            return rivi.info();
        }
    }
    case Qt::DecorationRole: {
        if( index.column() == AIKA) {
            if( rivi.status() == "OK") return QIcon(":/pic/ok.png");
            else if( rivi.status() == "ERROR") return QIcon(":/pic/virhe64.png");
            else if( rivi.status() == "WARNING") return QIcon(":/pic/varoitus.png");
            else return QIcon(":/pic/tyhja.png");
        }
    }
    default:
        return QVariant();
    }
}

void ExtraLogModel::lataa(const QVariantList &lista)
{
    beginResetModel();
    loki_.clear();
    for(const auto& item : lista) {
        loki_.append(EkstraLokiRivi(item.toMap()));
    }
    endResetModel();
}

ExtraLogModel::EkstraLokiRivi::EkstraLokiRivi(const QVariantMap &map)
{
    aika_ = map.value("timestamp").toDateTime().toString("dd.MM.yyyy HH.ss");
    QVariantMap data = map.value("data").toMap();
    Monikielinen info(data.value("info").toMap());
    info_ = info.teksti();
    status_ = data.value("status").toString();
}
