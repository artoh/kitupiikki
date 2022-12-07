#ifndef AVAUSKUUKAUSIMODEL_H
#define AVAUSKUUKAUSIMODEL_H


#include "tilinavausmodel.h"
#include "avauserakantamodel.h"

class AvausKuukausiModel : public AvausEraKantaModel
{
    Q_OBJECT

protected:
    class AvausKuukausi {
    public:
        AvausKuukausi(const QDate& pvm);

        Euro saldo() const;
        QDate pvm() const { return pvm_;}
        QList<AvausEra> erat() const { return erat_;}

        void lisaaEra(const AvausEra& era);
        void asetaEuro(const Euro& euro);
        void asetaErat(const QList<AvausEra> erat);
    private:
        QDate pvm_;
        QList<AvausEra> erat_;
    };


public:
    enum { KUUKAUSI, SALDO};

    explicit AvausKuukausiModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QList<AvausEra> erat() const override;
    Euro summa() const override;
    void lataa(QList<AvausEra> erat = QList<AvausEra>()) override;

protected:
    void alustaKuukaudet();

private:
    QList<AvausKuukausi> kuukaudet_;

};

#endif // AVAUSKUUKAUSIMODEL_H
