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
    AloitusSivu(QWidget *parent = nullptr);
    ~AloitusSivu() override;

    bool poistuSivulta(int minne) override;
    static QDate buildDate();

    bool eventFilter(QObject* target, QEvent* event) override;

public slots:
    void siirrySivulle() override;
    void paivitaSivu();
    void kirjanpitoVaihtui();

    void linkki(const QUrl& linkki);

    void uusiTietokanta();
    void avaaTietokanta();
    void tuoKitupiikista();

    void abouttiarallaa();

    void infoSaapui();
    void varmuuskopioi();

    void muistiinpanot();

    void poistaListalta();
    void poistaPilvesta();

    /**
     * @brief Pyytää infon päivityksistä
     */
    void pyydaInfo();

protected slots:
    void saldotSaapuu(QVariant* data);
    void inboxSaapuu(QVariant* data);
    void outboxSaapuu(QVariant* data);

private slots:
    void pilviLogin();
    void kirjauduttu();
    void loginVirhe();
    void validoiLoginTiedot();
    void validoiEmail();
    void emailTarkastettu();
    void verkkovirhe(QNetworkReply::NetworkError virhe);
    void vaihdaUnohtunutSalasana();
    void salasananVaihtoLahti();
    void pilviLogout();
    void logoMuuttui();
    void haeSaldot();
    void haeInOutBox();
    void siirraPilveen();    
    void tukiInfo();
    void lisaTukiInfo(QVariant *data);
    void vaihdaSalasanaUuteen();

signals:
    void selaus(int tilinumero, Tilikausi tilikausi);
    void ktpkasky(QString kasky);

protected:
    QString vinkit();
    QString summat();




    /**
     * @brief palvelimelta (kitupiikki.arkku.net) haettu tiedote uusista päivityksista tms.
     *
     * @since 0.2
     */
    QString paivitysInfo;

    enum PilviPino { KIRJAUDU, LISTA, SISAANTULO };

protected:
    Ui::Aloitus *ui;
    bool sivulla = false;
    bool kelpoEmail_=false;
    QVariantMap saldot_;
};

#endif // ALOITUSSIVU_H
