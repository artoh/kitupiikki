# Kitupiikin asentaminen

!!! info "Ei takuuta"
    Saat ohjelmiston käyttöösi täysin ilman maksua.
    Kitupiikki-ohjelmaan ei sisälly minkäänlaista takuuta ohjelman toimivuudesta tai soveltuvuudesta. Ohjelma on testattu huolellisesti, mutta siinä voi silti olla virheitä ja ohjelman käyttäjä ottaa vastuun ohjelman mahdollisesti aiheuttamista vahigoista.

## <span class="fa fa-windows"></span> Windows

Tuetut Windowsin versiot: Windows 7, 8 ja 10.

!!! note ""
    <span class="fa fa-windows"></span> [Kitupiikki 1.0 Windows asennusohjelma (11 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0/kitupiikki-1.0-asennus.exe) <i class="fa fa-laptop"></i> Lataa ja suorita asennusohjelma. <i class="fa fa-user"></i> Vaatii pääkäyttäjän oikeudet.

     <span class="fa fa-windows"></span> [Kitupiikki 1.0 Windows suoritettava ohjelma (13 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0/kitupiikki1.0.exe) <i class="fa fa-briefcase"></i> Käynnistettävissä ilman asennusta esim. USB-muistilta. Älä kuitenkaan talleta kirjanpitoa USB-muistille, sillä sen käsittely olisi hidasta.



## <span class="fa fa-linux"></span> Linux

Toimii 64-bittisissä Linux-jakeluissa joissa graafinen työpöytä (esim. Ubuntu 14.04 ja uudemmat)

!!! note ""
    <span class="fa fa-linux"></span> [Kitupiikki 1.0 Linux AppImage (30 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0/Kitupiikki-1.0-x86_64.AppImage)

Kitupiikin Linux-version toimitetaan AppImage-tiedostona, jota ei varsinaisesti edes tarvitse asentaa. Et tarvitse ylläpitäjän oikeuksia.

1. Lataa yllä oleva asennustiedosto
2. Merkitse tiedosto suoritettavaksi. Useimpien Linux-versioiden tiedostonhallinnassa se tehdään klikkaamalla tiedostoa hiiren oikealla napilla ja valitsemalla **Ominaisuudet**, ja ruksaamalla **Oikeudet**-välilehdeltä **Suoritettava**. Komentorivillä onnistuu komennolla `chmod u+x Kitupiikki*.AppImage`
3. Käynnistä ohjelma klikkaamalla tiedostoa tai komennolla `./Kitupiikki*.AppImage`
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
- Asunto-osakeyhtiön tilikartta <span class=ver>BETA.1</span>

Beta-versio asentuu Kitupiikin varsinaisen version rinnalle.

!!! warning ""
    <span class="fa fa-windows"></span> [Kitupiikki 1.1 BETA.1 Windows asennusohjelma (11 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.1-beta.1/kitupiikki-1.1-beta.1-asennus.exe) <span class="fa fa-exclamation-triangle"> </span> **Testikäyttöön**

    <span class="fa fa-linux"></span> [Kitupiikki 1.1 BETA.1 Linux AppImage (30 MB)](hhttps://github.com/artoh/kitupiikki/releases/download/v1.1-beta.1/Kitupiikki-1.1-beta.1-x86_64.AppImage) <span class="fa fa-exclamation-triangle"> </span>  **Testikäyttöön**

## Kehitysversio

Ohjelman viimeisimmän kehitysversion saat GitHubista <https://github.com/artoh/kitupiikki>. Kehitysversio pitää tietenkin kääntää itse, joten tarvitset [Qt-kirjastoa](http://qt.io).

## Versiohistoria

Ohjelman versiohistoria sekä aiemmat julkaistut versiot löytyvät GitHub-palvelusta <https://github.com/artoh/kitupiikki/releases>.
