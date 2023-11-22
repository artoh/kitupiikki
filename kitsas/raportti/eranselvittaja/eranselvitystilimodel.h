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
        EranSelvitysTili(const QVariantMap& map);

        int tili() const { return tili_;}
        Euro saldo() const { return saldo_;}
        Euro kausi() const { return kausi_;}
        int erittelemattomia() const { return erittelemattomia_;}
    protected:
        int tili_ = 0;
        Euro saldo_ = Euro::Zero;
        Euro kausi_ = Euro::Zero;
        int erittelemattomia_ = 0;
    };

public:
    explicit EranSelvitysTiliModel(QDate startDate, QDate date, QObject *parent = nullptr);

    enum { TILI, KAUSI, SALDO };
    enum { TiliNumeroRooli = Qt::UserRole } ;

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
    QDate startDate_;
    QDate date_;
};

#endif // ERANSELVITYSTILIMODEL_H
