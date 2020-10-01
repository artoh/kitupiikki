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

#include "model/tosite.h"

#include "naytin/esikatseltava.h"
#include "db/tositetyyppimodel.h"
#include "db/verotyyppimodel.h"

#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"



namespace Ui {
class LaskuDialogi;
}

class KohdennusDelegaatti;
class LaskuRivitModel;
class RyhmalaskuTab;
class EnnakkoHyvitysModel;

/**
 * @brief Laskun laatimisen dialogi
 */
class LaskuDialogi : public QDialog, public Esikatseltava
{
    Q_OBJECT
public:

    LaskuDialogi(const QVariantMap& data = QVariantMap(), bool ryhmalasku = false, int asiakas = 0);
    ~LaskuDialogi() override;

    enum Tabs { RIVIT, LISATIEDOT, RYHMAT};
    enum Lahetys { TULOSTETTAVA, SAHKOPOSTI, VERKKOLASKU, PDF, EITULOSTETA, POSTITUS, TUOTULASKU };
    enum Maksutapa { LASKU, KATEINEN, ENNAKKOLASKU, SUORITEPERUSTE };
    enum Marginaali { KAYTETYT =91, TAIDE =  92, ANTIIKKI=93 };

    void lisaaEnnakkoHyvitys(int eraId, double eurot);
    QString asiakkaanAlvTunnus() const { return asAlvTunnus_;}

private:
    void paivitaSumma();

    void paivitaNapit();

    void tulosta(QPagedPaintDevice *printer) const override;
    QString otsikko() const override;

    void lisaaTuote();
    void tuotteidenKonteksiValikko(QPoint pos);
    void tulostaLasku();

    void asiakasValittu(int asiakasId);
    void taytaAsiakasTiedot(QVariant* data);
    void asiakasHaettuLadattaessa(QVariant* data);
    void paivitaLaskutustavat();
    void laskutusTapaMuuttui();
    void maksuTapaMuuttui();

    QVariantMap data(QString otsikko = QString()) const;

    void tallenna(Tosite::Tila moodi);
    void tallennusValmis(QVariant* vastaus, bool toimita = false);

    int tyyppi() const { return tyyppi_;}
    void ennakkoHyvitysData(int eraid, double eurot, QVariant *data);

    void alustaRyhmalasku();
    void lataa(const QVariantMap& map);    
    void paivitaNakyvat();
    void alustaRiviTab();

    QVariantMap vastakirjaus(const QDate &pvm, const QString& otsikko) const;
    void alustaMaksutavat();
    void ohje();

    qlonglong laskeViivastysKorko() const;
    void taydennaMaksumuistutuksenData(QVariantMap& map) const;

    void naytaLoki();

signals:
    void tallennettuValmiina();

private:
    LaskuRivitModel *rivit_;
    Ui::LaskuDialogi *ui;    
            
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
    int asiakasId_ = 0;
    QDate alkupPvm_;

    int tyyppi_ = TositeTyyppi::MYYNTILASKU;

    RyhmalaskuTab *ryhmalaskuTab_ = nullptr;
    bool ryhmalasku_ = false;
    
    QVariantList aiemmat_;
    double aiempiSaldo_ = 0.0;

    EnnakkoHyvitysModel* ennakkoModel_;
    bool verkkolaskutettava_ = false;
    bool paivitetaanLaskutapoja_ = false;
};

#endif // UUSILASKUDIALOGI_H
