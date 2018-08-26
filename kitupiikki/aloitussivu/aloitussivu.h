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

/**
  * @dir aloitussivu
  * @brief Aloitussivu
  */

#ifndef ALOITUSSIVU_H
#define ALOITUSSIVU_H

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "db/tilikausi.h"
#include "kitupiikkisivu.h"

#include "ui_aloitus.h"



/**
 * @brief Webipohjaiset aloitus- ja ohjesivut
 *
 *
 */
class AloitusSivu : public KitupiikkiSivu
{
    Q_OBJECT

public:
    AloitusSivu();
    ~AloitusSivu();

public slots:

    void siirrySivulle();
    void kirjanpitoVaihtui();

    void linkki(const QUrl& linkki);

    void uusiTietokanta();
    void avaaTietokanta();
    void viimeisinTietokanta(QListWidgetItem* item);

    void abouttiarallaa();

    void infoSaapui(QNetworkReply* reply);
    void varmuuskopioi();

    /**
     * @brief Pyytää infon päivityksistä
     */
    void pyydaInfo();

signals:
    void selaus(int tilinumero, Tilikausi tilikausi);
    void ktpkasky(QString kasky);

protected:
    QString vinkit();
    QString summat();

    void saldot();
    void paivitaTiedostoLista();

    /**
     * @brief palvelimelta (kitupiikki.arkku.net) haettu tiedote uusista päivityksista tms.
     *
     * @since 0.2
     */
    QString paivitysInfo;

protected:
    Ui::Aloitus *ui;
};

#endif // ALOITUSSIVU_H
