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
#include <QInputDialog>

#include "model/tosite.h"
#include "ui_kirjaus.h"


class Kirjanpito;
class QAction;
class SelausWg;

class Tosite;
class ApuriWidget;
class KiertoWidget;
class KommentitWidget;
class TallennettuWidget;
class KirjausSivu;

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
    KirjausWg(KirjausSivu *parent=nullptr, QList<int> selauslista = QList<int>());
    ~KirjausWg();

    enum Valilehdet { VIENNIT, MUISTIINPANOT, LIITTEET, VARASTO, LOKI } ;

    Tosite* tosite() { return tosite_;}
    ApuriWidget* apuri() { return apuri_;}

public slots:
    void lataaTosite(int id);
    void lisaaUusiLiite();
    void lisaaLiite(const QString &polku);
    void lisaaLiiteDatasta(const QByteArray& data, const QString& nimi);

    void tyhjenna();

private:
    enum MuokkausSallinta { Lukittu = 0, AlvLukittu = 1, Sallittu = 2};

private slots:

    void valmis();
    void lisaaRivi();
    void paivitaOtsikkoSelitteeksi();
    void poistaRivi();

    void hylkaa();
    void poistaTosite();
    void vientiValittu();
    void uusiVienti();
    void muokkaaVientia();

    void paivitaKommentti(const QString& kommentti);    

    void salliMuokkaus(KirjausWg::MuokkausSallinta sallitaanko);

    void vaihdaTositeTyyppi();
    void tositeTyyppiVaihtui(int tyyppiKoodi);

    void tunnisteVaihtui(int tunniste);

    void paivitaSarja(bool kateinen = false);

    /**
     * @brief Liitetiedosto valittu, näytetään se
     * @param selected
     */
    void poistaLiite();


    /**
     * @brief Tositteen tulostaminen
     */
    void tulostaTosite();

    /**
     * @brief Valikon siirry-toiminta
     */
    void siirryTositteeseen();

    void naytaLoki();

    void pohjaksi();

    void tositeLadattu();

    void siirryEdelliseen(bool tallennuksesta = false);
    void siirrySeuraavaan(bool tallennuksesta = false);


private slots:    
    void paivita(bool muokattu, int virheet, const Euro& debet, const Euro& kredit);
    void tallenna(int tilaan);
    void tallennettu(int id, int tunniste, const QDate& pvm, const QString& sarja = QString(), int tila = Tosite::KIRJANPIDOSSA);
    void tallennusEpaonnistui(int virhe);

    void tuonti(const QVariantMap &map);

    void naytaKommenttimerkki(bool onko);
    void vaihdaTunniste();

public:
    Ui::KirjausWg* gui() { return ui;}

    /**
     * @brief Vientilistauksen nykyinen indeksi
     * @return
     */
    int nykyinenRivi() const { return ui->viennitView->currentIndex().row(); }

    void paivitaSelausLista(QList<int> lista);
    void nollaaTietokannanvaihtuessa();


signals:
    void liiteValittu(const QByteArray& pdf, bool salliPudotus);

    void tulostaLiite();
    void avaaLiite();
    void tallennaLiite();

    void naytaPohjat(bool nayta);

protected:
    /**
     * @brief Muokattuja toimintoja
     *
     * Pvm-kentästä eteenpäin enterillä
     * Seliteestä tablilla uusi rivi
     *
     * @param watched
     * @param event
     * @return
     */
    bool eventFilter(QObject *watched, QEvent *event);
    void paivitaLiiteNapit();


    void tarkastaTuplatJaTallenna(int tila);
    void tuplaTietoSaapuu(QVariant* data, int tila = Tosite::KIRJANPIDOSSA);

    bool tarkastaHylkays();
    KirjausSivu* kirjausSivu();

protected:
    enum Selauksesta { EI_SELAUKSESTA, EDELLISEEN, SEURAAVAAN };

    Ui::KirjausWg *ui;    

    QAction *poistaAktio_;
    QAction *uudeksiAktio_;
    QAction *tyhjennaViennitAktio_;
    QAction *mallipohjaksiAktio_;

    QSortFilterProxyModel *tyyppiProxy_;

    Tosite* tosite_;
    ApuriWidget* apuri_;

    QWidget* viennitTab_;
    QWidget* memoTab_;
    QWidget* liitteetTab_;
    QWidget* varastoTab_;
    QWidget* lokiTab_;
    KiertoWidget* kiertoTab_;
    KommentitWidget* kommentitTab_;

    QList<int> selausLista_;
    Selauksesta selauksesta_ = EI_SELAUKSESTA;
    KirjausSivu* kirjausSivu_;
};

#endif // KIRJAUSWG_H
