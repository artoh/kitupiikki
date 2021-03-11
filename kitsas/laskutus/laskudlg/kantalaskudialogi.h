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
#ifndef KANTALASKUDIALOGI_H
#define KANTALASKUDIALOGI_H

#include <QDialog>
#include <QVariantMap>

class Tosite;

class HuoneistoModel;

namespace Ui {
class LaskuDialogi;
}

class Lasku;
class EnnakkoHyvitysModel;

class KantaLaskuDialogi : public QDialog
{
    Q_OBJECT
public:
    KantaLaskuDialogi(Tosite* tosite, QWidget* parent = nullptr);
    ~KantaLaskuDialogi() override;

protected:
    Tosite* tosite() { return tosite_;}

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

private:
    void alustaUi();
    void alustaRivitTab();

protected:
    Ui::LaskuDialogi *ui;
    Tosite* tosite_;

    EnnakkoHyvitysModel* ennakkoModel_;

    int asiakasId_;
    QVariantMap ladattuAsiakas_;

    bool paivitetaanLaskutapoja_ = false;

private:
    HuoneistoModel* huoneistot_;
};


#endif // KANTALASKUDIALOGI_H
