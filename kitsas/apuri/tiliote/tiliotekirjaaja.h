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

class LaskuTauluTilioteProxylla;
class TilioteApuri;
class TilioteViennit;

class QSortFilterProxyModel;

namespace Ui {
class TilioteKirjaaja;
}

class TilioteKirjaaja : public QDialog
{
    Q_OBJECT    
public:
    enum YlaTab { TILILLE, TILILTA};
    enum AlaTab { MAKSU, TULOMENO, HYVITYS, SIIRTO, PIILOSSA };

    TilioteKirjaaja( TilioteApuri* apuri );
    ~TilioteKirjaaja() override;

    void asetaPvm(const QDate& pvm);    

public slots:
    void accept() override;
    void kirjaaUusia(const QDate& pvm = QDate());
    void muokkaaRivia(int riviNro);


private slots:
    void alaTabMuuttui(int tab);
    void euroMuuttuu();
    void ylaTabMuuttui(int tab);
    void tiliMuuttuu();
    void paivitaAlvInfo();
    void eraValittu(int eraId, double avoinna, const QString& selite);
    void jaksomuuttuu(const QDate& pvm);

    void valitseLasku();
    void suodata(const QString& teksti);

    void tyhjenna();
    void tarkastaTallennus();

    void kumppaniValittu(int kumppaniId);
    void kumppaniTiedot(QVariant* data);

    void haeAlkuperaisTosite(int eraId);
    void tositeSaapuu(QVariant *data);

    void lisaaVienti();
    void poistaVienti();
    void paivitaVientiNakyma();


private:
    TilioteApuri* apuri() const;
    void lataa(const TilioteKirjausRivi& rivi);
    void lataaNakymaan();

    void riviVaihtuu(const QModelIndex &current, const QModelIndex &previous);
    void naytaRivi();
    void tallennaRivi();
    TilioteKirjausRivi tallennettava() const;


private:    
    Ui::TilioteKirjaaja *ui;

    int riviIndeksi_ = -1;
    int menoa_ = false;
    int nykyVientiRivi_ = 0;

    QSortFilterProxyModel* maksuProxy_;    
    QSortFilterProxyModel* avoinProxy_;

    LaskuTauluTilioteProxylla *laskut_;

    QVariantList alkuperaisRivit_;

    TositeVienti pankkiVienti_;
    TilioteViennit* viennit_;

};

#endif // TILIOTEKIRJAAJA_H
