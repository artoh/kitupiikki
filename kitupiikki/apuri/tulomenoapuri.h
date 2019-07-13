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

class QSortFilterProxyModel;
class TmRivit;
class KohdennusProxyModel;

namespace Ui {
class TuloMenoApuri;
}

class TuloMenoApuri : public ApuriWidget
{
    Q_OBJECT    

public:
    enum Maksutapa { LASKU, PANKKI, LUOTTO, KATEINEN, HYVITYS, ENNAKKO };

    TuloMenoApuri(QWidget *parent = nullptr, Tosite* tosite = nullptr);
    virtual ~TuloMenoApuri() override;

    void otaFokus() override;

protected slots:
    void lisaaRivi();
    void poistaRivi();

    void tiliMuuttui();
    void verolajiMuuttui();
    void maaraMuuttui();
    void verotonMuuttui();
    void veroprossaMuuttui();
    void alvVahennettavaMuuttui();
    void seliteMuuttui();
    void maksutapaMuuttui();
    void kohdennusMuuttui();
    void merkkausMuuttui();
    void jaksoAlkaaMuuttui();
    void jaksoLoppuuMuuttui();

    void haeRivi(const QModelIndex& index);
    void haeKohdennukset();

protected:
    void teeReset() override;
    bool teeTositteelle() override;

    void alusta(bool meno);
    int rivilla() const;


private:
    Ui::TuloMenoApuri *ui;
    TmRivit* rivit_;

    qlonglong bruttoSnt_ = 0;
    qlonglong nettoSnt_ = 0;


    QSortFilterProxyModel* veroFiltteri_;
    KohdennusProxyModel* kohdennusProxy_;
};

#endif // TULOMENOAPURI_H
