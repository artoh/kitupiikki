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
    AsiakkaatModel(QObject *parent = 0, bool toimittajat = false);

    enum AsiakasSarake { NIMI, YHTEENSA, AVOINNA, ERAANTYNYT };

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    void paivita(bool toimittajat = false);

protected:
    QList<AsiakasRivi> rivit_;
    bool toimittajat_ = false;  /** Näyttää toimittajien tiedot */



};

#endif // ASIAKKAATMODEL_H
