#ifndef KOOSTERAPORTTILISTAMODEL_H
#define KOOSTERAPORTTILISTAMODEL_H

#include <QAbstractTableModel>

class KoosteRaporttiListaModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    class KoosteRaportti {

    public:
        KoosteRaportti();
        KoosteRaportti(const QVariantMap& data);

        QString ajalta() const { return ajalta_;}
        QString luotu() const { return luotu_;}
        QString luonut() const { return luonut_;}
        int id() const { return id_;}

    protected:
        QString ajalta_;
        QString luonut_;
        int id_;
        QString luotu_;

    };

public:
    enum { AJALTA, LAATINUT, LAADITTU};

    explicit KoosteRaporttiListaModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load();
    void dataSaapuu(const QVariant* data);

private:
    QList<KoosteRaportti> data_;

};

#endif // KOOSTERAPORTTILISTAMODEL_H
