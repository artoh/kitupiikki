#ifndef ERANSELVITYSVIENNIT_H
#define ERANSELVITYSVIENNIT_H

#include <QDate>
#include <QAbstractTableModel>

class EranSelvitysViennit : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EranSelvitysViennit(const QDate &alkuPvm, const QDate &loppuPvm, QObject *parent = nullptr);

    enum { PVM, TOSITE, SELITE, DEBET, KREDIT };

    enum { TositeIdRooli = Qt::UserRole,
           VientiMapRooli = Qt::UserRole + 1,
           PvmRooli = Qt::UserRole + 2,
           VientiIdRooli = Qt::UserRole + 3
    };

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


    void load(int tili, int eraid);
    void clear();
private:
    void dataSaapuu(QVariant* data);

    QVariantList data_;
    QDate alkuPvm_;
    QDate loppuPvm_;
};

#endif // ERANSELVITYSVIENNIT_H
