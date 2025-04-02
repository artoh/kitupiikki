#ifndef MAKSUTMODEL_H
#define MAKSUTMODEL_H

#include <QAbstractTableModel>

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

    explicit MaksutModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
};

#endif // MAKSUTMODEL_H
