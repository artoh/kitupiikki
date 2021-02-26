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
#ifndef TASETULOSRAPORTTI_H
#define TASETULOSRAPORTTI_H

#include "raporttiwidget.h"
#include "ui_muokattavaraportti.h"
#include "raportoija.h"

class TaseTulosRaportti : public RaporttiWidget
{
    Q_OBJECT
public:


    TaseTulosRaportti(Raportoija::RaportinTyyppi raportinTyyppi, QWidget* parent = nullptr);

public slots:
    void esikatsele() override;

protected slots:
    void muotoVaihtui();

    void paivitaKielet();
    void paivitaMuodot();

    void paivitaKohdennusPaivat();

protected:
    Raportoija::RaportinTyyppi tyyppi() const { return tyyppi_;}
    void paivitaUi();
    QString kaava_;

    Ui::MuokattavaRaportti *ui;
    Raportoija::RaportinTyyppi tyyppi_;

    bool paivitetaan_ = false;

};

#endif // TASETULOSRAPORTTI_H
