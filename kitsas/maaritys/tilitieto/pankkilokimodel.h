#ifndef TILITIETO_PANKKILOKIMODEL_H
#define TILITIETO_PANKKILOKIMODEL_H

#include <QAbstractTableModel>
#include <QVariantList>
#include <QList>

namespace Tilitieto {

class PankkiLokiModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    class LokiRivi
    {
    public:
        LokiRivi(const QVariantMap& map);
        QString aika() const { return aika_;}
        QString status() const { return status_;}
        QString iban() const { return iban_;}
    private:
        QString aika_;
        QString status_;
        QString iban_;
    };

    enum { AIKA, TILI, STATUS};


public:
    explicit PankkiLokiModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void lataa(const QVariantList &lista);

private:
    QList<LokiRivi> loki_;

    static QString statusTeksti(const QString& status);
};

} // namespace Tilitieto

#endif // TILITIETO_PANKKILOKIMODEL_H
