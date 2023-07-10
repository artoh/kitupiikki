#ifndef ERANSELVITYSTILIMODEL_H
#define ERANSELVITYSTILIMODEL_H

#include <QAbstractTableModel>

#include "model/euro.h"
#include "qdatetime.h"

class EranSelvitysTiliModel : public QAbstractTableModel
{
    Q_OBJECT

protected:
    class EranSelvitysTili {
    public:
        EranSelvitysTili();
        EranSelvitysTili(int tilinumero, Euro saldo);

        int tili() const { return tili_;}
        Euro saldo() const { return saldo_;}
    protected:
        int tili_ = 0;
        Euro saldo_ = Euro::Zero;
    };

public:
    explicit EranSelvitysTiliModel(QDate date, QObject *parent = nullptr);

    enum { TILI, SALDO };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void refresh();

private:
    void saldotSaapuu(QVariant* data);


    QList<EranSelvitysTili> data_;
    QDate date_;
};

#endif // ERANSELVITYSTILIMODEL_H
