/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef KOHDENNUSPROXYMODEL_H
#define KOHDENNUSPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QDate>
#include <QCursor>

/**
 * @brief Model, joka suodattaa vain aktiiviset kohdennukset
 */
class KohdennusProxyModel : public QSortFilterProxyModel
{
public:
    enum Naytettavat { KAIKKI, KOHDENNUKSET_PROJEKTIT, MERKKKAUKSET };

    /**
     * @brief Kohdennuksia suodattava proxy
     * @param parent
     * @param paiva Päivämäärä, jonka suhteen määräaikaisten suodatus
     * @param kohdennus Nykyisen kohdennukset id
     * @param naytetaan Minkä tyyppiset näytetään
     */
    KohdennusProxyModel(QObject *parent = nullptr, QDate paiva = QDate(), int kohdennus = -1, Naytettavat naytetaan = KOHDENNUKSET_PROJEKTIT);

    void asetaPaiva(const QDate& paiva) { nykyinenPaiva = paiva; invalidate(); }
    void asetaKohdennus(int kohdennus) { nykyinenKohdennus = kohdennus; invalidate(); }
    void asetaNaytettavat(Naytettavat naytetaan) { naytettavat = naytetaan; invalidate(); }


    /**
     * @brief Valikko tägien valitsemiseen
     * @param pvm Päivämäärä, jonka mukaan tägit valitaan
     * @param valitut Lista valittujen tagien id:stä
     * @return Valittujen tagit id-lista
     */
    static QVariantList tagiValikko(const QDate &pvm, QVariantList valitut = QVariantList(), QPoint sijainti = QCursor::pos());

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    QDate nykyinenPaiva;
    int nykyinenKohdennus;  // Nykykohdennus kelpaa aina
    Naytettavat naytettavat;
};

#endif // KOHDENNUSPROXYMODEL_H
