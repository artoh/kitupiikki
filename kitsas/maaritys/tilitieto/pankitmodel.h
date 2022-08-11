#ifndef TILITIETO_PANKITMODEL_H
#define TILITIETO_PANKITMODEL_H

#include <QAbstractListModel>
#include "pankki.h"

namespace Tilitieto {

class PankitModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum {
        IdRooli = Qt::UserRole,
        BicRooli = Qt::UserRole + 1
    };

    explicit PankitModel(QObject *parent = nullptr);


    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int indeksiBicilla(const QString& bic) const;

    Pankki pankki(int id) const;

    void haePankit();    

private:
    void haeLogot();
    void pankitSaapuu(QVariant* data);
    void haeSeuraavaLogo();
    void logoSaapuu();

    QList<Pankki> pankit_;
    int logoIndeksi_ = -1;

};

} // namespace Tilitieto

#endif // TILITIETO_PANKITMODEL_H
