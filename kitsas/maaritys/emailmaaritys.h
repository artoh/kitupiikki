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

#ifndef EMAILMAARITYS_H
#define EMAILMAARITYS_H

#include "maarityswidget.h"

#include "ui_emailmaaritys.h"

/**
 * @brief Sähköpostin lähettämiseen liittyvät määritykset
 */
class EmailMaaritys : public MaaritysWidget
{
    Q_OBJECT
public:
    EmailMaaritys();
    ~EmailMaaritys() override;

public:
    bool nollaa() override;
    bool tallenna() override;
    bool onkoMuokattu() override;

    QString ohjesivu() override { return "asetukset/sahkoposti";}

    static int sslIndeksi(const QString& asetus);
    static QString sslAsetus(int indeksi);

public slots:
    void ilmoitaMuokattu();
    void kokeile();
    void porttiVaihtui(int portti);
    void paivitaKitsasVaihto();

protected:
    void kokeileSmtp();
    void kokeileKitsas();
    void kitsasOnnistui();
    void kitsasEpaonnistui(int virhe, const QString& selitys);

private:

    Ui::EMailMaaritys *ui;
    bool paikallinen_=false;    

};

#endif // EMAILMAARITYS_H
