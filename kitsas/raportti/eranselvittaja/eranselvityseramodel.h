#ifndef ERANSELVITYSERAMODEL_H
#define ERANSELVITYSERAMODEL_H

#include <QAbstractTableModel>
#include <QDate>

#include "model/eramap.h"

class EranSelvitysEraModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EranSelvitysEraModel(QObject *parent = nullptr);

    enum { PVM, KUMPPANI, SELITE, SALDO};

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load(const int tili, const QDate& date);

private:
    void eratSaapuu(QVariant* data);

    QList<EraMap> erat_;
};

#endif // ERANSELVITYSERAMODEL_H
