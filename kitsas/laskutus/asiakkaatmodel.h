#ifndef ASIAKKAATMODEL_H
#define ASIAKKAATMODEL_H

#include <QAbstractTableModel>
#include <QList>

/**
 * @brief Asiakkaiden tai toimittajien model
 */
class AsiakkaatModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum KumppaniValinta { REKISTERI, ASIAKKAAT, TOIMITTAJAT};

    AsiakkaatModel(QObject *parent = nullptr,KumppaniValinta valinta = REKISTERI);

    enum AsiakasSarake { NIMI, YHTEENSA, AVOINNA, ERAANTYNYT };
    enum {
        IdRooli = Qt::UserRole,
        NimiRooli = Qt::UserRole + 1,
        MapRooli = Qt::UserRole + 2
    };

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    void paivita(int valinta);
    void suodataRyhma(int ryhmaId);

protected slots:
    void tietoSaapuu(QVariant* var);

protected:

    QVariantList lista_;

    int valinta_;



};

#endif // ASIAKKAATMODEL_H
