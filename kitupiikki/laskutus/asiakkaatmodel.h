#ifndef ASIAKKAATMODEL_H
#define ASIAKKAATMODEL_H

#include <QAbstractTableModel>
#include <QList>


/**
 * @brief Asiakkaan tiedot luettelossa
 */
struct AsiakasRivi
{
    AsiakasRivi() {}

    QString nimi;
    qlonglong yhteensa = 0;
    qlonglong avoinna = 0;
    qlonglong eraantynyt = 0;
};


/**
 * @brief Asiakkaiden tai toimittajien model
 */
class AsiakkaatModel : public QAbstractTableModel
{
public:
    AsiakkaatModel(QObject *parent = nullptr, bool toimittajat = false);

    enum AsiakasSarake { NIMI, YHTEENSA, AVOINNA, ERAANTYNYT };
    enum { IdRooli = Qt::UserRole, NimiRooli = Qt::UserRole + 1 };

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    void paivita(bool toimittajat = false);

protected slots:
    void tietoSaapuu(QVariant* var);

protected:
    QList<AsiakasRivi> rivit_;

    QVariantList lista_;

    bool toimittajat_ = false;  /** Näyttää toimittajien tiedot */



};

#endif // ASIAKKAATMODEL_H
