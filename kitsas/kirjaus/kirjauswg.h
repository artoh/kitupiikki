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


class Kirjanpito;
class QAction;
class SelausWg;

class Tosite;
class ApuriWidget;

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
    KirjausWg(QWidget *parent=nullptr, SelausWg* selaus = nullptr);
    ~KirjausWg();

    enum Valilehdet { VIENNIT, KOMMENTIT, LIITTEET, VARASTO, LOKI } ;

    Tosite* tosite() { return tosite_;}

public slots:
    void lataaTosite(int id);
    void lisaaLiite(const QString &polku);
    void lisaaLiiteDatasta(const QByteArray& data, const QString& nimi);

    void tyhjenna();

private slots:

    void valmis();
    void lisaaRivi();
    void poistaRivi();

    void hylkaa();
    void poistaTosite();
    void vientiValittu();

    void paivitaKommentti(const QString& kommentti);
    void salliMuokkaus(bool sallitaanko);

    void vaihdaTositeTyyppi();
    void tositeTyyppiVaihtui(int tyyppiKoodi);

    void tunnisteVaihtui(int tunniste);

    void paivitaSarja(bool kateinen = false);

    /**
     * @brief Liitetiedosto valittu, näytetään se
     * @param selected
     */
    void liiteValinta(const QModelIndex& valittu);
    void poistaLiite();

    /**
     * @brief Verojen ja tase-erien muokkaus
     * @param indeksi
     */
    void vientivwAktivoitu(QModelIndex indeksi);


    /**
     * @brief Tositteen tulostaminen
     */
    void tulostaTosite();

    /**
     * @brief Valikon siirry-toiminta
     */
    void siirryTositteeseen();

    void naytaLoki();

private slots:
    void paivita(bool muokattu, int virheet, double debet, double kredit);
    void tallennettu(int id, int tunniste, const QDate& pvm, const QString& sarja = QString());
    void tallennusEpaonnistui(int virhe);

    void tuonti(QVariant* data);

    void nollaaTietokannanvaihtuessa();

public:
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
     * Seliteestä tablilla uusi rivi
     *
     * @param watched
     * @param event
     * @return
     */
    bool eventFilter(QObject *watched, QEvent *event);
    void paivitaLiiteNapit();


protected:
    Ui::KirjausWg *ui;    

    QAction *poistaAktio_;
    QAction *uudeksiAktio_;
    QAction *tyhjennaViennitAktio_;

    QSortFilterProxyModel *tyyppiProxy_;

    Tosite* tosite_;
    ApuriWidget* apuri_;

    QWidget* viennitTab_;
    QWidget* kommentitTab_;
    QWidget* liitteetTab_;
    QWidget* varastoTab_;
    QWidget* lokiTab_;

    SelausWg* selaus_;
    QPair<int,int> edellinenSeuraava_;
};

#endif // KIRJAUSWG_H
