# Kitupiikin asentaminen

!!! info "Ei takuuta"
    Saat ohjelmiston käyttöösi täysin ilman maksua.
    Kitupiikki-ohjelmaan ei sisälly minkäänlaista takuuta ohjelman toimivuudesta tai soveltuvuudesta. Ohjelma on testattu huolellisesti, mutta siinä voi silti olla virheitä ja ohjelman käyttäjä ottaa vastuun ohjelman mahdollisesti aiheuttamista vahigoista.

## <span class="fa fa-windows"></span> Windows

Tuetut Windowsin versiot: Windows 7, 8 ja 10.

!!! note ""
    <span class="fa fa-windows"></span> [Kitupiikki 1.0 Windows asennusohjelma (11 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0/kitupiikki-1.0-asennus.exe) <i class="fa fa-laptop"></i> Lataa ja suorita asennusohjelma. <i class="fa fa-user"></i> Vaatii pääkäyttäjän oikeudet.

     <span class="fa fa-windows"></span> [Kitupiikki 1.0 Windows suoritettava ohjelma (13 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0/kitupiikki1.0.exe) <i class="fa fa-briefcase"></i> Käynnistettävissä ilman asennusta esim. USB-muistilta.


## <span class="fa fa-linux"></span> Linux

Toimii 64-bittisissä Linux-jakeluissa joissa graafinen työpöytä (esim. Ubuntu 16.04 ja uudemmat)

!!! note ""
    <span class="fa fa-linux"></span> [Kitupiikki 1.0 Linux AppImage (30 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.0/Kitupiikki-1.0-x86_64.AppImage)

Kitupiikin Linux-versio toimitetaan AppImage-tiedostona, jota ei varsinaisesti edes tarvitse asentaa. Et tarvitse ylläpitäjän oikeuksia.

1. Lataa yllä oleva asennustiedosto
2. Merkitse tiedosto suoritettavaksi. Useimpien Linux-versioiden tiedostonhallinnassa se tehdään klikkaamalla tiedostoa hiiren oikealla napilla ja valitsemalla **Ominaisuudet**, ja ruksaamalla **Oikeudet**-välilehdeltä **Suoritettava**. Komentorivillä onnistuu komennolla `chmod u+x Kitupiikki*.AppImage`
3. Käynnistä ohjelma klikkaamalla tiedostoa tai komennolla `./Kitupiikki*.AppImage`
4. Ensimmäisellä käynnistyskerralla Kitupiikki kysyy, haluatko lisätä ohjelman käynnistysvalikkoon.


## <span class="fa fa-apple"></span> Macintosh

Tukee OS X versiota 10.11 ja uudempia

!!! information "Mac-versiossa vielä hieman hiomista..."
    Macintosh-versiossa on vielä pieniä yhteensopivuusongelmia. Jotta Kitupiikki saataisiin toimimaan luotettavasti Macilla, olisi testausapu tervetullutta!

    Mac-version voi ladata osoitteesta [https://github.com/petriaarnio/kitupiikki/releases](https://github.com/petriaarnio/kitupiikki/releases) (ensimmäinen .dmg-päätteinen tiedosto) ja virheistä ilmoitella tämän sivun alareunan keskustelualueelle tai palaute@kitupiikki.info. <span class="fa fa-exclamation-triangle"> </span> **Beta-versio, saattaa sisältää vielä virheitä**

<!--
!!! note ""
    <span class="fa fa-linux"></span> [Kitupiikki 1.0 macOS asennuspaketti (15 MB) ](https://github.com/petriaarnio/kitupiikki/releases/download/mac_v1.0.0/Kitupiikki.dmg)  -->

1. Lataa yllä oleva asennustiedosto
2. Avaa asennustiedosto kaksoisnapsauttamalla hiirellä
3. Vedä avautuneesta ikkunasta Kitupiikin kuvake Ohjelmat (Applications) -hakemiston kuvakkeen päälle

Macintosh-julkaisua ylläpitää [Petri Aarnio](https://github.com/petriaarnio).


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
- Ryhmälaskutus <span class=ver>BETA.2</span>
- Finvoice-laskun muodostaminen <span class=ver>BETA.2</span>

Beta-versio käynnistyy suoraan ladattavasta tiedostosta, joten voit kokeilla sitä, vaikka käyttäisitkin muuten Kitupiikin vakaata versiota.

!!! warning ""
    <span class="fa fa-windows"></span> [Kitupiikki 1.1 BETA.4 Windows suoritettava ohjelma (14 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.1-beta.4/kitupiikki1.1-beta.4.exe) <span class="fa fa-exclamation-triangle"> </span> **Testikäyttöön**

    <span class="fa fa-linux"></span> [Kitupiikki 1.1 BETA.4 Linux AppImage (30 MB)](https://github.com/artoh/kitupiikki/releases/download/v1.1-beta.4/Kitupiikki-1.1-beta.4-x86_64.AppImage) <span class="fa fa-exclamation-triangle"> </span>  **Testikäyttöön**

## Kehitysversio

Ohjelman viimeisimmän kehitysversion saat GitHubista <https://github.com/artoh/kitupiikki>. Kehitysversio pitää tietenkin kääntää itse, joten tarvitset [Qt-kirjastoa](http://qt.io).

## Versiohistoria

Ohjelman versiohistoria sekä aiemmat julkaistut versiot löytyvät GitHub-palvelusta <https://github.com/artoh/kitupiikki/releases>.
