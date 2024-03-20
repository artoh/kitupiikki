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
#ifndef TILIOTEKIRJAAJA_H
#define TILIOTEKIRJAAJA_H

#include <QDialog>

#include "tiliotemodel.h"
#include "tiliotekirjausrivi.h"
#include "tiliotealirivitmodel.h"
#include "model/eramap.h"

class LaskuTauluTilioteProxylla;
class LaskuTauluModel;
class TilioteApuri;
class TilioteViennit;

class QSortFilterProxyModel;
class SiirtoApuri;

namespace Ui {
class TilioteKirjaaja;
}

class TilioteKirjaaja : public QDialog
{
    Q_OBJECT    
public:
    enum YlaTab { TILILLE, TILILTA};
    enum AlaTab { MAKSU, TULOMENO, HYVITYS, SIIRTO, VAKIOVIITE, PIILOSSA };

    TilioteKirjaaja( TilioteApuri* apuri );
    TilioteKirjaaja( SiirtoApuri* apuri);

    ~TilioteKirjaaja() override;

    void asetaPvm(const QDate& pvm);    
    QList<TositeVienti> viennit() const;

public slots:
    void accept() override;
    void kirjaaUusia(const QDate& pvm = QDate());
    void muokkaaRivia(int riviNro);


private slots:
    void alaTabMuuttui(int tab);

    void euroMuuttuu();
    void verotonMuuttuu();


    void alvMuuttuu();
    void alvProssaMuttuu();
    void alvVahennettavaMuuttuu();

    void ylaTabMuuttui(int tab);
    void tiliMuuttuu();    
    void eraValittu(EraMap era);
    void jaksomuuttuu(const QDate& pvm);

    void valitseLasku();
    void suodata(const QString& teksti);

    void tyhjenna();
    void tarkastaTallennus();

    void kumppaniTiedot(const QVariantMap &data);

    void haeAlkuperaisTosite(int eraId);
    void tositeSaapuu(QVariant *data);

    void lisaaVienti();
    void poistaVienti();
    void paivitaVientiNakyma();


private:
    void alusta();

    double alvProssa();

    TilioteApuri* apuri() const;
    void lataa(const TilioteKirjausRivi& rivi);
    void lataaNakymaan();

    void riviVaihtuu(const QModelIndex &current, const QModelIndex &previous);
    void naytaRivi();
    void tallennaRivi();    

    void tallenna();

    void paivitaVeroFiltteri(const int verokoodi);

    TilioteAliRivi *aliRivi();
    void aliRiviaMuokattu();
    int rivilla() const { return nykyAliRiviIndeksi_;}

private:    
    Ui::TilioteKirjaaja *ui;

    int riviIndeksi_ = -1;
    bool menoa_ = false;
    int nykyAliRiviIndeksi_ = 0;

    QSortFilterProxyModel* maksuProxy_;    
    QSortFilterProxyModel* avoinProxy_;
    QSortFilterProxyModel* veroFiltteri_;

    LaskuTauluModel *laskut_;
    QVariantList alkuperaisRivit_;

    TilioteKirjausRivi rivi_ = nullptr;
    TilioteAliRivitModel* aliRiviModel_ = nullptr;

    bool ladataan_ = false;

};

#endif // TILIOTEKIRJAAJA_H
