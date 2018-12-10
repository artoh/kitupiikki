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

#ifndef KIRJAUSWG_H
#define KIRJAUSWG_H

#include <QWidget>
#include <QSortFilterProxyModel>

#include "ui_kirjaus.h"
#include "naytaliitewg.h"

#include "db/tositemodel.h"



class Kirjanpito;
class LaskunMaksuDialogi;
class ApuriVinkki;
class QAction;
class QSqlQueryModel;

/**
 * @brief Kirjausten muokkaus
 *
 * Widget muodostaa KirjausSivun alapuoliskon.
 * Tietoja käsitellän TositeModel:in kautta.
 *
 */
class KirjausWg : public QWidget
{
    Q_OBJECT
public:
    KirjausWg(TositeModel *tositeModel, QWidget *parent=nullptr);
    ~KirjausWg();

    QDate tositePvm() const;

    enum Valilehdet { VIENNIT, KOMMENTIT, LIITTEET, TILIOTE, LISATIEDOT } ;

    TositeModel *model() { return model_;}

public slots:
    void lisaaRivi();
    void poistaRivi();
    void tyhjenna();
    void tallenna();
    void hylkaa();
    void poistaTosite();
    void vientiValittu();

    /**
     * @brief Tekee uuden tositteen tämän pohjalta
     */
    void uusiPohjalta();


    /**
     * @brief Päivittää debet- ja kredit summat ja jos ei täsmää ei tallennusnappi käytössä
     */
    void naytaSummat();
    void lataaTosite(int id);

    void paivitaKommenttiMerkki();
    /**
     * @brief Jos tunniste ei ole kelpo, värjätään se punaiseksi!
     */
    void paivitaTunnisteVari();

    /**
     * @brief Lisätään liite
     * @param polku Polku liitetiedostoon.
     */
    void lisaaLiite(const QString &polku);
    void lisaaLiite();
    void lisaaLiiteDatasta(const QByteArray& data, const QString& nimi);


    /**
     * @brief Siirtää lomakkeen tiedot modeliin
     */
    void tiedotModeliin();
    /**
     * @brief Hakee tiedot modelista lomakkeeseen;
     */
    void tiedotModelista();


    /**
     * @brief Määrää saako tositetta muokata
     *
     * Järjestelmätositteiden sekä päätettyjen tilikausien tositteiden muokkaamista ei sallita
     *
     * @param sallitaanko
     */
    void salliMuokkaus(bool sallitaanko);

    void vaihdaTositeTyyppi();

    /**
     * @brief Liitetiedosto valittu, näytetään se
     * @param selected
     */
    void liiteValinta(const QModelIndex& valittu);

    /**
     * @brief Näyttää kirjausapurin
     */
    void kirjausApuri();

    void pvmVaihtuu();

    void poistaLiite();

    /**
     * @brief Verojen ja tase-erien muokkaus
     * @param indeksi
     */
    void vientivwAktivoitu(QModelIndex indeksi);

    /**
     * @brief Näyttää laskun maksun valintadialogin ja kirjaa maksun
     */
    void kirjaaLaskunmaksu();

    void paivitaTallennaPoistaNapit();

    /**
     * @brief Päivittää lukituksen ja alvin varoitukset
     */
    void paivitaVaroitukset() const;

    /**
     * @brief Siirtää tositteiden numeroita eteenpäin
     *
     * #117
     */
    void numeroSiirto();

    /**
     * @brief Tositteen tulostaminen
     */
    void tulostaTosite();

    /**
     * @brief Valikon siirry-toiminta
     */
    void siirryTositteeseen();

    /**
     * @brief Päivittää otsikon täydennyksen
     * @param teksti
     */
    void paivitaOtsikonTaydennys(const QString& teksti);


public:
    /**
     * @brief Jos kirjataan tiliotetta, tiliotetilin id
     *
     * Tätä käytetään laskunmaksussa, jotta lasku tulisi kohdistetuksi tiliotteen tilille
     *
     * @return tiliotetilin id tai 0
     */
    int tiliotetiliId();

    Ui::KirjausWg* gui() { return ui;}

    /**
     * @brief Vientilistauksen nykyinen indeksi
     * @return
     */
    int nykyinenRivi() const { return ui->viennitView->currentIndex().row(); }

signals:
    void liiteValittu(const QByteArray& pdf);
    /**
     * @brief Yksi tosite on saatu käsiteltyä.
     *
     * Jos ollaan tultu selauksesta, palataan selaukseen
     */
    void tositeKasitelty();
    void avaaLiite();
    void tulostaLiite();

protected:
    /**
     * @brief Muokattuja toimintoja
     *
     * Pvm-kentästä eteenpäin enterillä
     * tagivalikko
     * Seliteestä tablilla uusi rivi
     *
     * @param watched
     * @param event
     * @return
     */
    bool eventFilter(QObject *watched, QEvent *event);
    void paivitaLiiteNapit();
    void paivitaTilioteIcon();


protected:
    Ui::KirjausWg *ui;    
    TositeModel *model_;
    LaskunMaksuDialogi *laskuDlg_;
    ApuriVinkki *apurivinkki_;

    QAction *poistaAktio_;
    QAction *uudeksiAktio_;
    QAction *tyhjennaViennitAktio_;

    QSqlQueryModel *taydennysSql_;
    QSortFilterProxyModel *tyyppiProxy_;


};

#endif // KIRJAUSWG_H
