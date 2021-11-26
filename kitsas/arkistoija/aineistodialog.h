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
#ifndef AINEISTODIALOG_H
#define AINEISTODIALOG_H

#include <QDialog>
#include <QHash>
#include <QQueue>


#include "db/tilikausi.h"
#include "raportti/raportinkirjoittaja.h"
#include "raportti/raporttivalinnat.h"

class QProgressDialog;
class QPagedPaintDevice;
class QPainter;

namespace Ui {
class AineistoDialog;
}

class AineistoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AineistoDialog(QWidget *parent = nullptr);
    ~AineistoDialog();

    void aineisto(const QDate& pvm, const QString& kieli = QString());

    void accept() override;

private:
    void lataaDialogi();
    void tallennaDialogi();

    void paivitaNimi();
    void vaihdaNimi();

    void tilaaRaportit();
//    void raporttiSaapuu(int raportti, RaportinKirjoittaja rk);
    void jatkaTositelistaan();
    void tulostaRaportit();

    void tositeListaSaapuu(QVariant *data);
    void tilaaSeuraavaTosite();

    void tositeSaapuu(QVariant* data);
    void tilaaLiite();
    void tilattuLiiteSaapuu(QVariant* data, const QString& tyyppi);

    void valmis();

    RaporttiValinnat raportti(const QString& tyyppi) const;
    void tilaaRaportti(RaporttiValinnat& valinnat);
    void raporttiSaapuu(const RaportinKirjoittaja& kirjoittaja, const RaporttiValinnat& valinnat);


    Ui::AineistoDialog *ui;

    Tilikausi tilikausi_;
    QVector<RaportinKirjoittaja> kirjoittajat_;
    QString kieli_;
    bool nimivaihdettu_ = false;

    int tilattuja_ = 0;
    int valmiita_ = 0;
    bool tilauskesken_ = true;

    QVariantList tositteet_;
    QVariantMap nykyTosite_;
    QQueue<QPair<int,QString>> liiteJono_;
    int sivu_ = 2;

    int tositepnt_ = 0;
    bool virhe_ = false;
    bool tulostaAlatunniste_ = true;

    int rivinkorkeus_;


    QProgressDialog* progress;
    QPagedPaintDevice *device;
    QPainter *painter = nullptr;
};

#endif // AINEISTODIALOG_H
