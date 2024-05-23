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
#include <QSortFilterProxyModel>

#include "db/tilikausi.h"
#include "kitupiikkisivu.h"

#include "pilvi/pilvikayttaja.h"
#include "ui_aloitus.h"


class LoginService;

/**
 * @brief Webipohjaiset aloitus- ja ohjesivut
 *
 *
 */
class AloitusSivu : public KitupiikkiSivu
{
    Q_OBJECT

    enum Tabs {
        TIETOKONE_TAB,
        PILVI_TAB,
        TUKI_TAB
    };

public:
    AloitusSivu(QWidget *parent = nullptr);
    ~AloitusSivu() override;

    bool poistuSivulta(int minne) override;

    bool eventFilter(QObject* target, QEvent* event) override;

public slots:
    void siirrySivulle() override;
    void kirjanpitoVaihtui();
    void haeSaldot();

    void linkki(const QUrl& linkki);

    void uusiTietokanta();
    void avaaTietokanta();
    void tuoKitupiikista();

    void abouttiarallaa();

    void varmuuskopioi();

    void muistiinpanot();

    void poistaListalta();
    void poistaPilvesta();

protected slots:
    void kpInfoSaapuu(QVariant* data);

private slots:    
    void kirjauduttu(const PilviKayttaja& kayttaja);
    void loginVirhe();

    void pilviLogout();
    void logoMuuttui();
    void haeKpInfo();
    void siirraPilveen();    
    void vaihdaSalasanaUuteen();

signals:
    void selaus(int tilinumero, Tilikausi tilikausi);
    void ktpkasky(QString kasky);

protected:
    void initLoginService();
    enum PilviPino { KIRJAUDU, LISTA, SISAANTULO };    

protected:
    Ui::Aloitus *ui;
    bool sivulla = false;
    bool kelpoEmail_=false;
    QSortFilterProxyModel* pilviProxy_ = nullptr;

    LoginService* login_;

};

#endif // ALOITUSSIVU_H
