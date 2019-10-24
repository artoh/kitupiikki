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

#ifndef TILINPAATTAJA_H
#define TILINPAATTAJA_H

#include <QDialog>

#include "db/tilikausi.h"

namespace Ui {
class TilinPaattaja;
}

class ArkistoSivu;

/**
 * @brief Tilinpäätösdialogi
 *
 * Poistojen tekeminen, tilikauden lukitseminen, tilinpäätöksen muokkaaminen,
 * esikatselu/tulostaminen ja lopulta vahvistaminen
 *
 */
class TilinPaattaja : public QDialog
{
    Q_OBJECT

public:
    explicit TilinPaattaja(Tilikausi kausi, ArkistoSivu* arkisto, QWidget *parent = nullptr);
    ~TilinPaattaja();

public slots:
    void paivitaDialogi();

private slots:
    void lukitse();
    void teePoistot();
    void teeJaksotukset();
    void muokkaa();
    void esikatsele();
    void vahvista();

protected slots:
    void dataSaapuu(QVariant* data);

signals:
    void lukittu(Tilikausi kausi);
    void vahvistettu();

private:
    Tilikausi tilikausi;
    ArkistoSivu *arkistosivu;
    Ui::TilinPaattaja *ui;
    QVariantMap data_;
};

#endif // TILINPAATTAJA_H
