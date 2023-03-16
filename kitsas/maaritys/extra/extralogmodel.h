#ifndef EXTRALOGMODEL_H
#define EXTRALOGMODEL_H

#include <QAbstractTableModel>

class ExtraLogModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    class EkstraLokiRivi
    {
    public:
        EkstraLokiRivi(const QVariantMap& map = QVariantMap());
        QString aika() const { return aika_;}
        QString info() const { return info_;}
        QString status() const { return status_;}
    private:
        QString aika_;
        QString info_;
        QString status_;
    };

    enum { AIKA, INFO };

public:
    explicit ExtraLogModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lataa(const QVariantList& lista);

private:
    QList<EkstraLokiRivi> loki_;
};

#endif // EXTRALOGMODEL_H
