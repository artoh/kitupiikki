#ifndef LISAOSALOKIMODEL_H
#define LISAOSALOKIMODEL_H

#include <QAbstractTableModel>

class LisaosaLokiModel : public QAbstractTableModel
{
    Q_OBJECT


private:
    class LisaosaLokiRivi
    {
    public:
        LisaosaLokiRivi();
        LisaosaLokiRivi(const QVariantMap& data);

        QString aika() const { return aika_;}
        QString viesti() const { return viesti_;}
        QString status() const { return status_;}

    private:
        QString aika_;
        QString viesti_;
        QString status_;
    };

public:
    enum { AIKA, INFO };

    explicit LisaosaLokiModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lataa(const QString& addonId);

private:
    void dataSaapuu(QVariant* data);

    QList<LisaosaLokiRivi> loki_;
};

#endif // LISAOSALOKIMODEL_H
