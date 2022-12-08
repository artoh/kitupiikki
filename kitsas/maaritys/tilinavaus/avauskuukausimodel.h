#ifndef AVAUSKUUKAUSIMODEL_H
#define AVAUSKUUKAUSIMODEL_H


#include "tilinavausmodel.h"
#include "avauserakantamodel.h"
#include "tilinavausmodel.h"

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
    enum { KUUKAUSI, SALDO, ERITTELY};

    explicit AvausKuukausiModel(QObject *parent = nullptr, TilinavausModel::Erittely kohdennukset = TilinavausModel::EI_ERITTELYA);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QList<AvausEra> erat() const override;
    Euro summa() const override;
    void lataa(QList<AvausEra> erat = QList<AvausEra>()) override;

    QList<AvausEra> kuukaudenErat(int kuukausi) const;
    void asetaKuukaudenErat(const int kuukausi, const QList<AvausEra> erat);

    TilinavausModel::Erittely erittely() const { return kohdennukset_;}
protected:
    void alustaKuukaudet();

private:
    QList<AvausKuukausi> kuukaudet_;
    TilinavausModel::Erittely kohdennukset_ = TilinavausModel::EI_ERITTELYA;

};

#endif // AVAUSKUUKAUSIMODEL_H
