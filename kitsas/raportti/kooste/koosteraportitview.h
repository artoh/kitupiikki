#ifndef KOOSTERAPORTITVIEW_H
#define KOOSTERAPORTITVIEW_H

#include <QTableView>

class KoosteRaportitView : public QTableView
{
    Q_OBJECT
public:
    KoosteRaportitView();

protected:
    void naytaRaportti(const QModelIndex& index);
};

#endif // KOOSTERAPORTITVIEW_H
