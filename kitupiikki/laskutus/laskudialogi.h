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

#include "tuotemodel.h"

#include "smtp.h"
#include "model/tosite.h"

#include "naytin/esikatseltava.h"
#include "db/tositetyyppimodel.h"

namespace Ui {
class LaskuDialogi;
}

class KohdennusDelegaatti;
class LaskuRivitModel;

/**
 * @brief Laskun laatimisen dialogi
 */
class LaskuDialogi : public QDialog, public Esikatseltava
{
    Q_OBJECT
public:

    LaskuDialogi(const QVariantMap& data = QVariantMap());
    ~LaskuDialogi() override;

    enum Tabs { RIVIT, LISATIEDOT, RYHMAT};
    enum Lahetys { TULOSTETTAVA, SAHKOPOSTI, VERKKOLASKU, PDF, EITULOSTETA };
    enum Maksutapa { LASKU, KATEINEN };

    static int laskuIkkunoita();    


private slots:
    void paivitaSumma();
    void esikatselu();

    void paivitaNapit();

    /**
     * @brief Tulostuksen toteutus
     *
     * Tukee ryhmälaskutusta
     *
     * @param printer
     */
    void tulosta(QPagedPaintDevice *printer) const override;
    QString otsikko() const override;
    /**
     * @brief Finvoice-verkkolaskun muodostaminen
     */
    void perusteVaihtuu();



    void rivienKontekstiValikko(QPoint pos);

    void poistaLaskuRivi();

    void tuotteidenKonteksiValikko(QPoint pos);

    void tulostaLasku();
    void ryhmaNapit(const QItemSelection& valinta);

    void lisaaAsiakasListalta(const QModelIndex& indeksi);
    void lisaaAsiakas();
    void tuoAsiakkaitaTiedostosta();
    void poistaValitutAsiakkaat();
    void paivitaRyhmanTallennusNappi();

    void verkkolaskuKayttoon();

    /////// UUTTA ///////
    void asiakasValittu(int asiakasId);
    void taytaAsiakasTiedot(QVariant* data);
    void paivitaLaskutustavat();
    void laskutusTapaMuuttui();
    void maksuTapaMuuttui();

    QVariantMap data() const;

    void tallenna(Tosite::Tila moodi);
    void tallennusValmis(QVariant* vastaus);

    int tyyppi() const { return tyyppi_;}

private:
    void lataa(const QVariantMap& map);
    void paivitaNakyvat();
    void lisaaRiviTab();
    QVariantMap vastakirjaus(const QString& otsikko) const;

    void alustaMaksutavat();



public slots:

private:
    LaskuRivitModel *rivit_;
    Ui::LaskuDialogi *ui;
            
    QModelIndex kontekstiIndeksi;
    KohdennusDelegaatti *kohdennusDelegaatti;

    QSortFilterProxyModel *ryhmaProxy_;

    QList<int> ryhmaLahetys_;

    QVariantMap tallennettu_;

    int tositeId_ = 0;
    qlonglong laskunnumero_ = 0l;
    QString viite_;
    QString asAlvTunnus_;
    int tunniste_ = 0;
    int era_ = 0;
    int alkupLasku_ = 0;
    QDate alkupPvm_;

    int tyyppi_ = TositeTyyppi::MYYNTILASKU;
    Tosite::Tila tallennusTila_ = Tosite::POISTETTU;
    
};

#endif // UUSILASKUDIALOGI_H
