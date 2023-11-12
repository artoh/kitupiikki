#ifndef LISAOSALISTMODEL_H
#define LISAOSALISTMODEL_H

#include <QAbstractListModel>
#include "lisaosat/lisaosa.h"

class LisaosaListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum { IdRooli = Qt::UserRole + 1,
           NimiRooli = Qt::UserRole + 2,
           AktiivinenRooli = Qt::UserRole + 3 };

    explicit LisaosaListModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void load();

protected:
    void dataSaapuu(QVariant* data);

private:
    QList<Lisaosa> lisaosat_;
};

#endif // LISAOSALISTMODEL_H
