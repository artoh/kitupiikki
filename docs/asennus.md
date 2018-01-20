# Kitupiikin asentaminen

!!! warning "Ei takuuta"
    Saat ohjelmiston käyttöösi täysin ilman maksua.
    Kitupiikki-ohjelmaan ei sisälly minkäänlaista takuuta ohjelman toimivuudesta tai soveltuvuudesta. Ohjelma on testattu huolellisesti, mutta siinä voi silti olla virheitä ja ohjelman käyttäjä ottaa vastuun ohjelman mahdollisesti aiheuttamista vahigoista.

## Windows

!!! note ""

    <span class="fa fa-windows"></span> [Kitupiikki 0.3 beta Windows asennusohjelma](https://github.com/artoh/kitupiikki/releases/download/v0.3.0-beta/kitupiikki0.3-beta-asennus.exe) Testausvaiheessa oleva ohjelma, virheet ja puutteet vielä todennäköisiä

Lataa Kitupiikin asennusohjelma yllä olevasta linkistä ja suorita se. Asennus ei tarvitse ylläpitäjän oikeuksia.

!!! bug "Yhteensopivuusongelmia Edgen kanssa"
    Erilaisten raporttien avautuminen Microsoft Edgeen Windows 10 -järjestelmässä ei aina toimi. Saadaksesi raportit näkyviin vaihda pdf-tiedostojen näyttäjäksi esimerkiksi Chrome-selain.

## Linux

!!! note ""
    <span class="fa fa-linux"></span> [Kitupiikki 0.3 beta Linux AppImage](https://github.com/artoh/kitupiikki/releases/download/v0.3.0-beta/Kitupiikki-0.3-beta-x86_64.AppImage) Testausvaiheessa oleva ohjelma, virheet ja puutteet vielä todennäköisiä

Kitupiikin Linux-version toimitetaan AppImage-tiedostona, jota ei varsinaisesti edes tarvitse asentaa. Et tarvitse ylläpitäjän oikeuksia. Vaatii 64-bittisen Linuxin graafisella käyttöliittymällä, testattu toimivaksi Ubuntu 16.04 ja Linux Mint -jakeluissa.

1. Lataa yllä oleva asennustiedosto
2. Merkitse tiedosto suoritettavaksi. Useimpien Linux-versioiden tiedostonhallinnassa se tehdään klikkaamalla tiedostoa hiiren oikealla napilla ja valitsemalla **Ominaisuudet**, ja ruksaamalla **Oikeudet**-välilehdeltä **Suoritettava**. Komentorivillä onnistuu komennolla `chmod u+x Kitupiikki*.AppImage`
3. Käynnistä ohjelma klikkaamalla tiedostoa tai komennolla `./Kitupiikki*.AppImage`


## Kehitysversio

Ohjelman viimeisimmän kehitysversion saat GitHubista <https://github.com/artoh/kitupiikki>. Kehitysversio pitää tietenkin kääntää itse, joten tarvitset [Qt-kirjastoa](http://qt.io).
