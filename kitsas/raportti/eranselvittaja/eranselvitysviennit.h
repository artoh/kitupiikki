#ifndef ERANSELVITYSVIENNIT_H
#define ERANSELVITYSVIENNIT_H

#include <QAbstractTableModel>

class EranSelvitysViennit : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EranSelvitysViennit(QObject *parent = nullptr);

    enum { PVM, TOSITE, SELITE, DEBET, KREDIT };

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
};

#endif // ERANSELVITYSVIENNIT_H
