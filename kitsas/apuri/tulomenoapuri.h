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
#ifndef TULOMENOAPURI_H
#define TULOMENOAPURI_H

#include "apuriwidget.h"
#include "model/euro.h"

class QSortFilterProxyModel;
class TmRivit;
class KohdennusProxyModel;
class QCompleter;
class TulomenoRivi;
class MaksutapaModel;

namespace Ui {
class TuloMenoApuri;
}

class TuloMenoApuri : public ApuriWidget
{
    Q_OBJECT    

public:
    TuloMenoApuri(QWidget *parent = nullptr, Tosite* tosite = nullptr);
    virtual ~TuloMenoApuri() override;

    void otaFokus() override;
    void tuo(QVariantMap map) override;

public slots:
    void salliMuokkaus(bool sallitaanko = true) override;

protected slots:
    void lisaaRivi();
    void poistaRivi();

    void tiliMuuttui();
    void verolajiMuuttui();

    void pvmMuuttui(const QDate& pvm);
    void maaraMuuttui();
    void verotonMuuttui();
    void veroprossaMuuttui();
    void alvVahennettavaMuuttui();
    void seliteMuuttui();
    void maksutapaMuuttui();    
    void vastatiliMuuttui();
    void kohdennusMuuttui();
    void merkkausMuuttui();
    void jaksoAlkaaMuuttui();
    void jaksoLoppuuMuuttui();
    void poistoAikaMuuttuu();
    void paivitaVeroFiltterit(const QDate& pvm);

    void haeRivi(const QModelIndex& index);
    void haeKohdennukset();

    void kumppaniValittu(int kumppaniId);
    void kumppaniTiedot(QVariant* data);
    void eraValittu(int eraid, Euro avoinna, const QString& selite, int kumppani);

protected:
    void teeReset() override;
    bool teeTositteelle() override;

    void alusta(bool meno);
    int rivilla() const;
    TulomenoRivi *rivi();

    void setAlvProssa(double prosentti);
    double alvProssa() const;

    void vastaSaldoSaapuu(QVariant* data);

    bool eventFilter(QObject *target, QEvent* event) override;

private:
    Ui::TuloMenoApuri *ui;
    TmRivit* rivit_;

    bool menoa_ = false;
    bool tuonnissa_ = false;

    QSortFilterProxyModel* veroFiltteri_;
    MaksutapaModel *maksutapaModel_;

    QString viimeMaksutapa_;
    QString arkistotunnus_;

};

#endif // TULOMENOAPURI_H
