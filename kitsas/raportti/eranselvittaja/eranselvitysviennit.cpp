#include "eranselvitysviennit.h"

#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"

EranSelvitysViennit::EranSelvitysViennit(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant EranSelvitysViennit::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case PVM: return tr("Päivämäärä");
        case TOSITE: return tr("Tosite");
        case SELITE: return tr("Selite");
        case DEBET: return tr("Debet");
        case KREDIT: return tr("Kredit");
        }
    }
    return QVariant();
}

int EranSelvitysViennit::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return data_.count();
}

int EranSelvitysViennit::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 5;
}

QVariant EranSelvitysViennit::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = data_.at(index.row()).toMap();

    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case PVM:
            return map.value("pvm").toDate().toString("dd.MM.yyyy");
        case TOSITE:
        {
            QVariantMap tositeMap = map.value("tosite").toMap();
            return tositeMap.value("sarja").toString() + tositeMap.value("tunniste").toString();
        }
        case SELITE:
            return map.value("selite").toString();
        case DEBET:
            return Euro(map.value("debet").toString()).display(false);
        case KREDIT:
            return Euro(map.value("kredit").toString()).display(false);
        }
    } else if( role == Qt::TextAlignmentRole && index.column() >= DEBET) {
        return Qt::AlignRight;
    } else if( role == Qt::UserRole) {
        return map.value("tosite").toMap().value("id").toInt();
    } else if( role == Qt::DisplayRole && index.column() == TOSITE) {
        const int tositeTyyppi = map.value("tosite").toMap().value("tyyppi").toInt();
        return kp()->tositeTyypit()->kuvake(tositeTyyppi);
    }

    return QVariant();
}

void EranSelvitysViennit::load(int tili, int eraid)
{
    KpKysely* kysely = kpk("/viennit");
    kysely->lisaaAttribuutti("tili", tili);
    kysely->lisaaAttribuutti("era", eraid);
    connect( kysely, &KpKysely::vastaus, this, &EranSelvitysViennit::dataSaapuu);
    kysely->kysy();
}

void EranSelvitysViennit::clear()
{
    beginResetModel();
    data_.clear();
    endResetModel();
}

void EranSelvitysViennit::dataSaapuu(QVariant *data)
{
    beginResetModel();
    data_ = data->toList();
    endResetModel();
}
