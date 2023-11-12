#include "lisaosalokimodel.h"

#include <QDateTime>
#include <QIcon>
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikayttaja.h"

LisaosaLokiModel::LisaosaLokiModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant LisaosaLokiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case AIKA: return tr("Aika");
        case INFO: return tr("Tapahtuma");
        }
    }
    return QVariant();
}

int LisaosaLokiModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return loki_.count();
}

int LisaosaLokiModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 2;
}

QVariant LisaosaLokiModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const LisaosaLokiRivi& rivi = loki_.at(index.row());
    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case AIKA:
            return rivi.aika();
        case INFO:
            return rivi.viesti();
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

void LisaosaLokiModel::lataa(const QString &addonId)
{
    KpKysely* kysely = kp()->pilvi()->loginKysely("/v1/addons/log");
    kysely->lisaaAttribuutti("addonId", addonId);
    kysely->lisaaAttribuutti("bookId", kp()->pilvi()->pilvi().bookId());
    connect( kysely, &KpKysely::vastaus, this, &LisaosaLokiModel::dataSaapuu);
    kysely->kysy();
}

void LisaosaLokiModel::dataSaapuu(QVariant *data)
{
    QVariantList lista = data->toList();
    beginResetModel();
    loki_.clear();
    for(const auto& item : lista) {
        loki_.append(LisaosaLokiRivi(item.toMap()));
    }
    endResetModel();
}

LisaosaLokiModel::LisaosaLokiRivi::LisaosaLokiRivi()
{

}

LisaosaLokiModel::LisaosaLokiRivi::LisaosaLokiRivi(const QVariantMap &data)
{
    aika_ = data.value("timestamp").toDateTime().toLocalTime().toString("dd.MM.yyyy HH.mm");
    viesti_ = data.value("message").toString();
    status_ = data.value("status").toString();
}
