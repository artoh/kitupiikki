#include "maksutmodel.h"

MaksutModel::MaksutModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

QVariant MaksutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case DATE_COLUMN: return tr("Maksupäivä");
        case AMOUNT_COLUMN: return tr("Määrä");
        case REF_COLUMN: return tr("Viite/Viesti");
        case STATUS_COLUMN: return tr("Tila");
        }
    }
    // FIXME: Implement me!
    return QVariant();
}

int MaksutModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return 2;
}

int MaksutModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return 4;
}

QVariant MaksutModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const bool firstLine = index.row() == 0;
    if( role == Qt::DisplayRole) {
        switch (index.column()) {
        case DATE_COLUMN:
            return firstLine ? "07.04.2025" : "14.04.2025";
        case AMOUNT_COLUMN:
            return firstLine ? "120,42" : "1 230,45";
        case REF_COLUMN:
            return firstLine ? "1009" : "Viesti saajalle";
        case STATUS_COLUMN:
            return firstLine ? "Hyväksymättä" : "Käsittelyssä";
        }
    }

    if( role == Qt::TextAlignmentRole && index.column() == AMOUNT_COLUMN)
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);

    // FIXME: Implement me!
    return QVariant();
}
