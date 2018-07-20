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

#ifndef UUSILASKUDIALOGI_H
#define UUSILASKUDIALOGI_H

#include <QDialog>
#include <QSortFilterProxyModel>

#include "laskumodel.h"
#include "tuotemodel.h"
#include "laskuntulostaja.h"
#include "laskutmodel.h"

#include "smtp.h"

namespace Ui {
class LaskuDialogi;
}

class KohdennusDelegaatti;

/**
 * @brief Laskun laatimisen dialogi
 */
class LaskuDialogi : public QDialog
{
    Q_OBJECT
public:

    LaskuDialogi(LaskuModel *laskumodel = 0);
    ~LaskuDialogi();

private slots:
    void paivitaSumma(qlonglong paivitaSumma);
    void esikatsele();
    void perusteVaihtuu();
    void haeOsoite();

    /**
     * @brief Tallentaa lomaketiedot malliin
     */
    void vieMalliin();

    void rivienKontekstiValikko(QPoint pos);
    void lisaaTuoteluetteloon();

    void lisaaTuote(const QModelIndex& index);
    void poistaLaskuRivi();

    void tuotteidenKonteksiValikko(QPoint pos);
    void poistaTuote();
    void paivitaTuoteluetteloon();

    void onkoPostiKaytossa();
    void lahetaSahkopostilla();
    void smtpViesti(const QString &viesti);
    void tulostaLasku();

private:
    /**
     * @brief Näyttäää tuoteluettelon jos tuotteita, muuten ohjeen
     */
    void paivitaTuoteluettelonNaytto();

public slots:
    void accept();
    void reject();

private:

    LaskuModel *model;
    TuoteModel *tuotteet;
    Ui::LaskuDialogi *ui;
    
    LaskunTulostaja *tulostaja;
    
    QSortFilterProxyModel *tuoteProxy;

    QModelIndex kontekstiIndeksi;
    KohdennusDelegaatti *kohdennusDelegaatti;
    
};

#endif // UUSILASKUDIALOGI_H
