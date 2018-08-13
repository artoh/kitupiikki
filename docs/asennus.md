# Kitupiikin asentaminen

!!! info "Ei takuuta"
    Saat ohjelmiston käyttöösi täysin ilman maksua.
    Kitupiikki-ohjelmaan ei sisälly minkäänlaista takuuta ohjelman toimivuudesta tai soveltuvuudesta. Ohjelma on testattu huolellisesti, mutta siinä voi silti olla virheitä ja ohjelman käyttäjä ottaa vastuun ohjelman mahdollisesti aiheuttamista vahigoista.

## <span class="fa fa-windows"></span> Windows

Tuetut Windowsin versiot: Windows 7, 8 ja 10.

!!! note ""
    <span class="fa fa-windows"></span> [Kitupiikki 1.0 RC.2 Windows asennusohjelma (11 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0-rc.2/kitupiikki-1.0-rc.2-asennus.exe) <span class="fa fa-exclamation-triangle"></span> Julkaisuehdokas-versio

Lataa Kitupiikin asennusohjelma yllä olevasta linkistä ja suorita se. Asennushakemisto on vapaasti valittavissa, joten asennus ei tarvitse ylläpitäjän oikeuksia.

![](images/asennus_hakemisto.png)

## <span class="fa fa-linux"></span> Linux

Toimii 64-bittisissä Linux-jakeluissa joissa graafinen työpöytä (esim. Ubuntu 14.04 ja uudemmat)

!!! note ""
    <span class="fa fa-linux"></span> [Kitupiikki 1.0 RC.2 Linux AppImage (30 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0-rc.2/Kitupiikki-1.0-rc.2-x86_64.AppImage) <span class="fa fa-exclamation-triangle"></span> Julkaisuehdokas -versio

Kitupiikin Linux-version toimitetaan AppImage-tiedostona, jota ei varsinaisesti edes tarvitse asentaa. Et tarvitse ylläpitäjän oikeuksia.

1. Lataa yllä oleva asennustiedosto
2. Merkitse tiedosto suoritettavaksi. Useimpien Linux-versioiden tiedostonhallinnassa se tehdään klikkaamalla tiedostoa hiiren oikealla napilla ja valitsemalla **Ominaisuudet**, ja ruksaamalla **Oikeudet**-välilehdeltä **Suoritettava**. Komentorivillä onnistuu komennolla `chmod u+x Kitupiikki*.AppImage`
3. Käynnistä ohjelma klikkaamalla tiedostoa tai komennolla `./Kitupiikki*.AppImage`
   ![](images/tervetuloa.png)
4. Ensimmäisellä käynnistyskerralla Kitupiikki kysyy, haluatko lisätä ohjelman käynnistysvalikkoon.


## <span class="fa fa-exclamation-triangle"></span> Kitupiikki Beta

!!! warning "Testikäyttöön"
    Beta-versiota ei testattu kattavasti, joten siinä on todennäköisesti vielä virheitä ja puutteita.

Version 1.1 uusia ominaisuuksia

- Uusittu laskunäkymä, jossa asiakas- ja toimittajarekisterit
- Laskun muokkaaminen
- Maksumuistutukset
- Budjetin laatiminen ja seuranta
- Kirjattavien tositteiden kansio

Beta-versio asentuu Kitupiikin varsinaisen version rinnalle.

!!! warning ""
    <span class="fa fa-windows"></span> [Kitupiikki 1.1 BETA Windows asennusohjelma (11 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.1-beta/kitupiikki-1.1-beta-asennus.exe) <span class="fa fa-exclamation-triangle"> </span> **Testikäyttöön**

    <span class="fa fa-linux"></span> [Kitupiikki 1.1 BETA Linux AppImage (30 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.1-beta/Kitupiikki-1.1-beta-x86_64.AppImage) <span class="fa fa-exclamation-triangle"> </span>  **Testikäyttöön**        

## Kehitysversio

Ohjelman viimeisimmän kehitysversion saat GitHubista <https://github.com/artoh/kitupiikki>. Kehitysversio pitää tietenkin kääntää itse, joten tarvitset [Qt-kirjastoa](http://qt.io).

## Versiohistoria

Ohjelman versiohistoria sekä aiemmat julkaistut versiot löytyvät GitHub-palvelusta <https://github.com/artoh/kitupiikki/releases>.
