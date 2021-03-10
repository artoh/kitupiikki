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
#ifndef LASKUDIALOGI_H
#define LASKUDIALOGI_H

#include "lasku.h"

#include <QDialog>

namespace Ui {
class LaskuDialogi;
}

class Lasku;
class EnnakkoHyvitysModel;

class LaskuDialogi : public QDialog
{
    Q_OBJECT
public:
    LaskuDialogi(QWidget* parent = nullptr);
    ~LaskuDialogi() override;

    void lataa(const QVariantMap& data);
    void lataa(const int tositeId);
    void asetaAsiakas(const int asiakas);

protected:
    void alustaUi();
    void alustaRivitTab();
    virtual void poistaLiikaTab();
    void teeConnectit();
    void alustaMaksutavat();
    void alustaValvonta();

    void tuotteidenKonteksiValikko(QPoint pos);

    void tositteelta();
    void asiakasMuuttui();
    void taytaAsiakasTiedot(QVariant* data);

    void paivitaLaskutustavat();
    void laskutusTapaMuuttui();
    void maksuTapaMuuttui();
    void valvontaMuuttui();
    void paivitaViiteRivi();

    void naytaLoki();

protected:
    Ui::LaskuDialogi *ui;
    Lasku lasku_;
    EnnakkoHyvitysModel* ennakkoModel_;

    QVariantMap ladattuAsiakas_;

    bool paivitetaanLaskutapoja_ = false;
};


#endif // LASKUDIALOGI_H
