#ifndef TILIOTEALIRIVITMODEL_H
#define TILIOTEALIRIVITMODEL_H

#include "apuri/tiliote/tiliotekirjausrivi.h"
#include <QAbstractTableModel>

class TilioteAliRivitModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum { TILI, ALV, EURO };
    TilioteAliRivitModel(KitsasInterface* interface, TilioteKirjausRivi* kirjausRivi, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    TilioteAliRivi rivi(int indeksi) const;

    void tyhjenna();
    void korvaa(int rivi, const TilioteAliRivi& uusi);
    void poista(int indeksi);
    void uusiRivi();

private:
    TilioteKirjausRivi* kirjausRivi_;
    KitsasInterface* kitsasInterface_;
};

#endif // TILIOTEALIRIVITMODEL_H
