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

#ifndef SISALTO_H
#define SISALTO_H

#include <QWebEnginePage>

/**
 * @brief Webipohjaisen sivun sisällön käsittely
 *
 * Apuluokka aloitus- ja ohjesivujen näyttämisessä.
 * lisaaTxt, lisaaLaatikko ja valmis -funktiot aloitussivun koostamiseen
 * ohjesivut ladataan load-funktiolla
 *
 * Selainta on siten muokattu, että ktp: ja avaa: -alkuiset osoitteet välitetetään
 * toiminto-signaalilla
 */
class Sisalto : public QWebEnginePage
{
    Q_OBJECT
public:
    Sisalto();

    /**
     * @brief Lisää html-tekstiä aloitussivun pääpalstalle
     * @param teksti Lisättävä html-teksti
     */
    void lisaaTxt(const QString& teksti);
    /**
     * @brief Lisää laatikon aloitussivun pääpalstalle
     * @param otsikko Laatikon ylälaidan otsikko (html)
     * @param sisalto Laatikon sisältö (html)
     */
    void lisaaLaatikko(const QString& otsikko, const QString& sisalto);
    /**
     * @brief Näyttää valmiiksi kirjoitetun sivun
     */
    void valmis();

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);

signals:
    /**
     * @brief Käyttäjä on valinnut aloitussivulta toiminnon
     * @param toimintonimi toiminto (avaa,uusi) tai avattavan tiedoston koko polku
     */
    void toiminto(const QString& toimintonimi);

protected:
    QString txt;

};

#endif // SISALTO_H
