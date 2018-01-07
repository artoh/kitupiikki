# Kitupiikin asentaminen

!!! warning "Ei takuuta"
    Saat ohjelmiston käyttöösi täysin ilman maksua.
    Kitupikki-ohjelmaan ei sisälly minkäänlaista takuuta ohjelman toimivuudesta tai soveltuvuudesta. Ohjelma on testattu huolellisesti, mutta siinä voi silti olla virheitä ja ohjelman käyttäjä ottaa vastuun ohjelman mahdollisesti aiheuttamista vahigoista.

!!! note "Ei vielä valmis"
    Kitupiikin lataustiedostot julkaistaan tällä sivulla ennen pitkää...

## Windows

!!! note ""
    <span class="fa fa-windows"></span> **[Tähän tulee asennuslinkki...](https://github.com/artoh/kitupiikki)** Ohjelman vakaa tuotantoversio

    <span class="fa fa-windows"></span> [Tähän tulee asennuslinkki...](https://github.com/artoh/kitupiikki) Testausvaiheessa oleva versio uusilla toiminnoilla

Lataa Kitupiikin asennustiedosto yllä olevasta linkistä ja suorita se. Asennus ei tarvitse ylläpitäjän oikeuksia.

(linkki asennusvideoon)

!!! bug "Yhteensopivuusongelmia Egden kanssa"
    Erilaisten raporttien avautuminen Microsoft Edgeen Windows 10 -järjestelmässä ei aina toimi. Saadaksesi raportit näkyviin vaihda pdf-tiedostojen näyttäjäksi esimerkiksi Chrome-selain.

## Linux

!!! note ""
    <span class="fa fa-linux"></span> [Tähän tulee asennuslinkki...](https://github.com/artoh/kitupiikki)

Kitupiikin Linux-version toimitetaan AppImage-tiedostona, jota ei varsinaisesti edes tarvitse asentaa. Et tarvitse ylläpitäjän oikeuksia. Vaatii 64-bittisen Linuxin graafisella käyttöliittymällä, testattu toimivaksi Ubuntu 16.04 ja Linux Mint -jakeluissa.

1. Lataa yllä oleva asennustiedosto
2. Merkitse tiedosto suoritettavaksi. Useimpien Linux-versioiden tiedostonhallinnassa se tehdään klikkaamalla tiedostoa hiiren vasemmalla napilla ja valitsemalla **Ominaisuudet**, ja ruksaamalla **Oikeudet**-välilehdeltä **Suoritettava**. Komentorivillä onnistuu komennolla `chmod u+x Kitupiikki*.AppImage`
3. Käynnistä ohjelma klikkaamalla tiedostoa tai komennolla `./Kitupiikki*.AppImage`

(linkki asennusvideoon)

## Kehitysversio

Ohjelman viimeisimmän kehitysversion saat GitHubista <https://github.com/artoh/kitupiikki>. Kehitysversio pitää tietenkin kääntää itse, joten tarvitset [Qt-kirjastoa](http://qt.io).
