#include "maksutmodel.h"
#include "db/kirjanpito.h"
#include "maksatusitem.h"

MaksutModel::MaksutModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

QVariant MaksutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case DATE_COLUMN: return tr("Maksupäivä");
        case AMOUNT_COLUMN: return tr("Määrä");
        case IBAN_COLUMN: return tr("IBAN");
        case REF_COLUMN: return tr("Viite/Viesti");
        case STATUS_COLUMN: return tr("Tila");
        }
    }
    return QVariant();
}

int MaksutModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return data_.count();
}

int MaksutModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 5;
}

QVariant MaksutModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if( role == Qt::DisplayRole) {
        const MaksatusItem item = data_.at(index.row());
        switch (index.column()) {
        case DATE_COLUMN:
            return item.pvm();
        case AMOUNT_COLUMN:
            return item.euro().display(true);
        case IBAN_COLUMN: {
            return item.iban().valeilla();
        }
        case REF_COLUMN:
            return item.viiteTaiViesti();
        case STATUS_COLUMN:
            return tilaTeksti(item.tila());
        }
    } else if( role == Qt::TextAlignmentRole && index.column() == REF_COLUMN ) {
        const auto& item = data_.at(index.row());
        // Tasataan viitenumerot oikealle mutta viestit vasemmalle
        if( !item.viite().isEmpty())
            return QVariant( Qt::AlignRight | Qt::AlignVCenter);
    } else if( role == Qt::TextAlignmentRole && index.column() == AMOUNT_COLUMN) {
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    } else if( role == Qt::DecorationRole ) {
        const MaksatusItem& item = data_.at(index.row());
        switch(index.column()) {
            case IBAN_COLUMN: return evaluationIcon(item.evaluation());
            case STATUS_COLUMN: return tilaIcon( item.tila() );
            default: return QVariant();
        }
    } else if( role == RejectableRole) {
        const auto& item = data_.at(index.row());
        return item.tila() == MaksatusItem::PENDING;
    } else if( role == IdRole ) {
        const auto& item = data_.at(index.row());
        return item.id();
    }

    // FIXME: Implement me!
    return QVariant();
}

void MaksutModel::load(const int documentId)
{
    const QString& url = QString("/maksut/%1").arg(documentId);
    KpKysely* kysely = kpk(url);
    connect(kysely, &KpKysely::vastaus, this, &MaksutModel::loadReady);
    kysely->kysy();
}

QString MaksutModel::tilaTeksti(const MaksatusItem::MaksatusTila tila)
{
    switch (tila) {
    case MaksatusItem::MaksatusTila::PENDING:
        return tr("Vahvistamatta");
    case MaksatusItem::MaksatusTila::PENDING_CONFIRMATION:
        return tr("Vahvistus kesken");
    case MaksatusItem::MaksatusTila::INPROGRESS:
        return tr("Käsittelyssä");
    case MaksatusItem::MaksatusTila::PAID:
        return tr("Maksettu");
    case MaksatusItem::MaksatusTila::REJECTED:
        return tr("Hylätty");
    case MaksatusItem::MaksatusTila::SCHEDULED:
        return tr("Odottaa maksupäivää");
    case MaksatusItem::MaksatusTila::ERROR:
        return tr("Hylätty (virhe)");
    default:
        return QString();
    }
}

QIcon MaksutModel::tilaIcon(const MaksatusItem::MaksatusTila tila)
{
    switch(tila) {
        case MaksatusItem::MaksatusTila::PENDING:
            return QIcon(":/pic/oranssi.png");
        case MaksatusItem::MaksatusTila::PENDING_CONFIRMATION:
            return QIcon(":/pic/keltainen.png");
        case MaksatusItem::MaksatusTila::INPROGRESS:
            return QIcon(":/pic/keltainen.png");
        case MaksatusItem::MaksatusTila::PAID:
            return QIcon(":/pic/ok.png");
        case MaksatusItem::MaksatusTila::REJECTED:
            return QIcon(":/pic/sulje.png");
        case MaksatusItem::MaksatusTila::SCHEDULED:
            return QIcon(":/pic/keltainen.png");
        case MaksatusItem::MaksatusTila::ERROR:
            return QIcon(":/pic/peru.png");
        default:
            return QIcon(":/pic/tyhja.png");
    }
}

QIcon MaksutModel::evaluationIcon(const MaksatusItem::IbanEvaluation evaluation)
{
    switch (evaluation) {
    case MaksatusItem::IbanEvaluation::EXISTING:
        return QIcon(":/pic/ok.png");
    case MaksatusItem::IbanEvaluation::CHANGED:
        return QIcon(":/pic/varoitus.png");
    default:
        return QIcon(":/pic/lisaa.png");
    }
}

void MaksutModel::loadReady(const QVariant *reply)
{
    const auto& list = reply->toList();

    beginResetModel();
    data_.clear();

    for(const auto& item : std::as_const(list)) {
        MaksatusItem mItem(item);
        data_.append( mItem );
    }

    endResetModel();
}
