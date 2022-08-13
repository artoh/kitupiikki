#ifndef TILITIETO_PANKKILOKIMODEL_H
#define TILITIETO_PANKKILOKIMODEL_H

#include <QAbstractTableModel>
#include <QVariantList>
#include <QList>
#include <QDate>

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
        int docId() const { return docId_; }
        QDate mista() const { return mista_;}
        QDate mihin() const { return mihin_;}
        bool system() const { return system_;}
    private:
        QString aika_;
        QString status_;
        QString iban_;        
        int docId_;
        QDate mista_;
        QDate mihin_;
        bool system_;
    };

    enum { AIKA, AJALTA, TILI, IBAN, STATUS};


public:

    enum { DocumentIdRole = Qt::UserRole + 1};

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
