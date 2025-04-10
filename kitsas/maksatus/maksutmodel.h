#ifndef MAKSUTMODEL_H
#define MAKSUTMODEL_H

#include <QAbstractTableModel>
#include "maksatusitem.h"


class MaksutModel : public QAbstractTableModel
{
    Q_OBJECT

public:

    enum {
        DATE_COLUMN,
        AMOUNT_COLUMN,
        REF_COLUMN,
        STATUS_COLUMN
    };

    enum {
        RejectableRole = Qt::UserRole + 1,
        IdRole = Qt::UserRole + 2
    };

    explicit MaksutModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const int documentId);

    static QString tilaTeksti(const MaksatusItem::MaksatusTila tila);
    static QIcon tilaIcon(const MaksatusItem::MaksatusTila tila);
private:
    void loadReady(const QVariant* reply);


    QList<MaksatusItem> data_;
};

#endif // MAKSUTMODEL_H
