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
    ~AloitusSivu() override;

    bool poistuSivulta(int minne) override;

public slots:
    void siirrySivulle() override;
    void kirjanpitoVaihtui();

    void linkki(const QUrl& linkki);

    void uusiTietokanta();
    void avaaTietokanta();

    void abouttiarallaa();

    void infoSaapui(QNetworkReply* reply);
    void varmuuskopioi();

    void muistiinpanot();

    void poistaListalta();

    /**
     * @brief Pyytää infon päivityksistä
     */
    void pyydaInfo();

    static QDate buildDate();

    void pilviLogin();
    void kirjauduttu();
    void loginVirhe();
    void validoiLoginTiedot();
    void emailTarkastettu();
    void rekisteroi();
    void rekisterointiLahti();
    void pilviLogout();

signals:
    void selaus(int tilinumero, Tilikausi tilikausi);
    void ktpkasky(QString kasky);

protected:
    QString vinkit();
    QString summat();

    QPair<QString,qlonglong> summa(const QString& otsikko, const QString& tyyppikysely, const Tilikausi& tilikausi, bool kreditplus = false, bool vali=false);

    void saldot();
    void paivitaTiedostoLista();

    /**
     * @brief palvelimelta (kitupiikki.arkku.net) haettu tiedote uusista päivityksista tms.
     *
     * @since 0.2
     */
    QString paivitysInfo;

    enum PilviPino { KIRJAUDU, LISTA };

protected:
    Ui::Aloitus *ui;
    bool sivulla = false;
};

#endif // ALOITUSSIVU_H
