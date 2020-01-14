/*
   Copyright (C) 2018 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BUDJETTIMODEL_H
#define BUDJETTIMODEL_H

#include <QAbstractTableModel>
#include <QMap>
#include <QDate>

class QSortFilterProxyModel;

/**
 * @brief Budjetin model
 *
 * Yhden tilikauden budjetti yhdelle kohdennukselle.
 * Budjetti tallennetaan tilikauden json-tietueeseen Budjetti-kenttään, joka
 * sisältää QVarianMapillisen (avaimena kohdennuksen id) QVariantMappeja
 * (avaimena tilinumero ja arvona budjetti sentteinä). Menot syötetään negatiivisina
 * lukuina.
 *
 * @since 1.1
 *
 */
class BudjettiModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Sarake
    {
        NRO, NIMI, EDELLINEN, EUROT
    };

    BudjettiModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    bool onkoMuokattu() const { return muokattu_; }

public slots:
    void lataa(const QDate& paivamaara);
    void nayta(int kohdennus);

    void tallenna();
    void laskeSumma();

    /**
     * @brief Kopioi edellisen tilikauden budjetin tämän tilikauden budjetin pohjaksi
     */
    void kopioiEdellinen();

private slots:
    void dataSaapuu(QVariant* saapuva);
    void edellinenSaapuu(QVariant* saapuva);
    void tallennettu();

signals:
    void summaMuuttui(qlonglong summa, qlonglong kokosumma);

protected:
    QSortFilterProxyModel *proxy_;
    QVariantMap data_;
    QVariantMap edellinen_;

    QDate paivamaara_;    
    int kohdennusid_ = 0;

    bool muokattu_ = false;


};

#endif // BUDJETTIMODEL_H
