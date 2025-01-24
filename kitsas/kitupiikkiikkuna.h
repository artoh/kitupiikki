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

#ifndef KITUPIIKKIIKKUNA_H
#define KITUPIIKKIIKKUNA_H

#include <QMainWindow>
#include <QMap>
#include <QVector>
#include <QStack>

class QStackedWidget;
class QAction;
class QActionGroup;
class QToolBar;
class AloitusSivu;
class SelausWg;
class KiertoSivu;
class RaporttiSivu;
class MaaritysSivu;
class ArkistoSivu;
class LaskuSivu;
class AlvSivu;
class OnniWidget;
class HubToimistoSivu;
class LisaosaSivu;

class QDateEdit;
class QDockWidget;

class TallennettuWidget;

#include "db/tilikausi.h"
#include "kitupiikkisivu.h"
#include "db/kirjanpito.h"

#include "pilvi/pilvikayttaja.h"
#include "kirjaus/kirjaussivu.h"

/**
 * @brief Ohjelmiston pääikkuna
 *
 * Pääikkunassa on vasemmalla työkalurivi, josta toimintosivu valitaan
 *
 */
class KitupiikkiIkkuna : public QMainWindow
{
    Q_OBJECT
public:
    KitupiikkiIkkuna(QWidget *parent = nullptr);
    ~KitupiikkiIkkuna();

    enum Sivu { ALOITUSSIVU, KIRJAUSSIVU, SELAUSSIVU, KIERTOSIVU, LASKUTUSSIVU, TULOSTESIVU, ARKISTOSIVU, ALVSIVU, MAARITYSSIVU, LISAOSASIVU, HUBTOIMISTOSIVU, MAJAVASIVU, SIVUT_LOPPU    };

signals:

public slots:
    /**
     * @brief Vaihtaa näytettävän sivun
     * @param mikasivu Mikä sivu näytetään (enum)
     */
    void valitseSivu(int mikasivu, bool paluu = false, bool siirry=true);

    void paivitaAktiivisuudet();

    void palaaSivulta();

    void selaaTilia(int tilinumero, const Tilikausi &tilikausi);

    void uusiKirjausIkkuna();
    void uusiSelausIkkuna();
    void uusiLasku();
    void naytaToimisto(const QString& id);    

    void naytaTallennettu(int tunnus, const QDate& paiva, const QString& sarja, int tila);

protected slots:
    void aktivoiSivu(QAction* aktio);
    void naytaTosite(int tositeid, QList<int> lista = QList<int>(), KirjausSivu::Takaisinpaluu paluu = KirjausSivu::PALATAAN_AINA);
    void ktpKasky(const QString &kasky);

    /**
     * @brief Näyttää lyhyen ajan viestiä onnistumisesta
     * @param teksti Näytettävä teksti
     */
    void naytaOnni(const QString& teksti, Kirjanpito::Onni tyyppi = Kirjanpito::Onnistui);

    /**
     * @brief Avaa ohjeen selaimeen
     */
    void ohje();

    void siirryTositteeseen();

    /**
     * @brief Kirjaa ensimmäisen tositteen Kirjattavien kansiosta
     */
    void kirjaaKirjattavienKansiosta();

    void paivitaPossu();

protected:
    void mousePressEvent(QMouseEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);
    void closeEvent(QCloseEvent* event);
    void resizeEvent(QResizeEvent* event);

protected:

    QStackedWidget *pino;
    QToolBar *toolbar;
    QActionGroup *aktioryhma;

    QAction* sivuaktiot[SIVUT_LOPPU ];
    KitupiikkiSivu* sivut[SIVUT_LOPPU ];

    QDockWidget *harjoitusDock;
    QDockWidget *inboxDock;

    TallennettuWidget* tallennettuWidget_;

    AloitusSivu *aloitussivu;
    KirjausSivu *kirjaussivu;
    LaskuSivu *laskutussivu;
    SelausWg *selaussivu;
    KiertoSivu* kiertosivu;
    RaporttiSivu *raporttisivu;
    MaaritysSivu *maarityssivu;
    ArkistoSivu *arkistosivu;
    AlvSivu *alvsivu;
    LisaosaSivu *lisaosaSivu;    
    HubToimistoSivu* hubToimistoSivu;
    HubToimistoSivu* majavaSivu;

    KitupiikkiSivu *nykysivu;

    QStack<int> edellisetIndeksit;

    QAction* uusiKirjausAktio;
    QAction* uusiSelausAktio;
    QAction* uusiLaskuAktio;    

    OnniWidget* onni_;



protected:
    QAction *lisaaSivu(const QString& nimi, const QString& kuva,
                          const QString& vihje, const QString& pikanappain, Sivu sivutunnus, KitupiikkiSivu *sivu);
    void lisaaSivut();

    void luoHarjoitusDock();
    void luoInboxDock();

    void suljeIkkunat();


};

#endif // KITUPIIKKIIKKUNA_H
