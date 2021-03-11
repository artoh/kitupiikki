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
#ifndef TAVALLINENLASKUDIALOGI_H
#define TAVALLINENLASKUDIALOGI_H

#include <QDialog>
#include <QVariantMap>

class Tosite;

namespace Ui {
class LaskuDialogi;
}

class Lasku;
class EnnakkoHyvitysModel;

class TavallinenLaskuDialogi : public QDialog
{
    Q_OBJECT
public:
    TavallinenLaskuDialogi(Tosite* tosite, QWidget* parent = nullptr);
    ~TavallinenLaskuDialogi() override;

protected:
    Tosite* tosite() { return tosite_;}

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
    Tosite* tosite_;

    EnnakkoHyvitysModel* ennakkoModel_;

    int asiakasId_;
    QVariantMap ladattuAsiakas_;

    bool paivitetaanLaskutapoja_ = false;
};


#endif // TAVALLINENLASKUDIALOGI_H
