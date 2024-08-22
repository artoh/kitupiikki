/*
   Copyright (C) 2019 Arto Hyvättinen

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
#ifndef TILIOTEKIRJAUSRIVI_H
#define TILIOTEKIRJAUSRIVI_H

#include "apuri/apuririvit.h"
#include "tilioterivi.h"
#include "taydennysviennit.h"
#include "apuri/apuririvi.h"

class TilioteKirjausRivi : public TilioteRivi
{
public:

    TilioteKirjausRivi(TilioteModel* model);
    TilioteKirjausRivi(const QVariantList& data, TilioteModel* model);
    TilioteKirjausRivi(const QVariantMap& tuonti, TilioteModel* model);
    TilioteKirjausRivi(const QDate& pvm, TilioteModel* model);

    QVariant riviData(int sarake, int role, const QDate& alkuPvm = QDate(), const QDate& loppuPvm = QDate()) const;
    bool setRiviData(int sarake, const QVariant& value);
    Qt::ItemFlags riviFlags(int sarake) const;

    void peita(bool onko) { peitetty_ = onko;}
    bool peitetty() const { return peitetty_;}
    bool tuotu() const { return tuotu_;}

    QString pseudoarkistotunnus() const;
    void alkuperaistositeSaapuu(QVariant* data, int eraId);

    void asetaLisaysIndeksi(const int indeksi) override;
    void paivitaErikoisrivit();

    Euro summa() const;
    QDate pvm() const { return paivamaara_; }
    QString otsikko() const { return otsikko_;}
    QString viite() const { return viite_;}
    QDate ostoPvm() const { return ostoPvm_;}
    QVariantMap kumppani() const { return kumppani_;}
    TositeVienti::VientiTyyppi tyyppi() const { return tyyppi_;}
    QString arkistotunnus() const { return arkistotunnus_;}

    int kohdennus() const;

    void asetaPvm(const QDate& pvm) { paivamaara_ = pvm;}
    void asetaOtsikko(const QString& otsikko) { otsikko_ = otsikko;}
    void asetaKumppani(const QVariantMap& map) { kumppani_ = map;}
    void asetaTyyppi(const TositeVienti::VientiTyyppi tyyppi) { tyyppi_ = tyyppi;}
    void asetaViite(const QString& viite) { viite_ = viite;}
    void asetaArkistotunnus(const QString& tunnus) { arkistotunnus_ = tunnus;}

    void lisaaVienti(const QVariantMap& map);

    QList<ApuriRivi> rivit() const { return rivit_;}
    void asetaRivit(ApuriRivit* rivit);


    QVariantList viennit(const int tilinumero) const;

protected:
    void paivitaTyyppi();
    void paivitaTyyppi(const EraMap& era, const int tilinumero);        

    QList<int> kirjausTilit() const;


    bool peitetty_ = false;
    bool tuotu_ = false;

    Euro summa_;
    QList<ApuriRivi> rivit_;

    TaydennysViennit taydennys_;

    TositeVienti::VientiTyyppi tyyppi_ = TositeVienti::TUNTEMATON;
    QDate paivamaara_;
    QString otsikko_;
    QVariantMap kumppani_;
    QString arkistotunnus_;
    QString viite_;
    QDate ostoPvm_;
    int vientiId_ = 0;
    QVariantMap tilioteLisatieto_;

};

#endif // TILIOTEKIRJAUSRIVI_H
