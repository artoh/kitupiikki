# Kitupiikki / Kitsas

[![versio](https://img.shields.io/github/release/artoh/kitupiikki.svg?label=Julkaistu%20versio)](https://github.com/artoh/kitupiikki/releases)
[![versio](https://img.shields.io/github/release/artoh/kitupiikki/all.svg?label=Esiversio)](https://github.com/artoh/kitupiikki/releases)
[![lisenssi](https://img.shields.io/github/license/artoh/kitupiikki.svg?label=Lisenssi)](https://raw.githubusercontent.com/artoh/kitupiikki/master/LICENSE)
[![mathdown HuBoard](https://img.shields.io/github/issues/artoh/kitupiikki.svg?label=Tehtävät%20%28HuBoard%29)](https://huboard.com/artoh/kitupiikki)

**Finnish bookkeeping software for small organisations**

Comments, variable names, documentations and the software itself are, of course, in Finnish only!


**Suomalainen avoimen lähdekoodin kirjanpito-ohjelma**

Kotisivu ja käyttöohjeet [https://kitupiikki.info](https://kitupiikki.info) sekä uuden version osalta [https://kitsas.fi](kitsas.fi)

![Kitsas](https://raw.githubusercontent.com/artoh/kitupiikki/kitsas/kitupiikki/pic/kitsas150.png)

## Kitsas on uusi Kitupiikki

Ohjelmassa ollaan siirtymässä versioon 2, jolloin ohjelman nimi lyhenee ja on jatkossa Kitsas. Kakkosversio elää **kitsas**-nimisessä haarassa. Ohjelman arkkitehtuuri uudistuu kokonaisuudessaan niin, että ohjelmassa on valmius kirjanpidon tallentamiseen omaa pilvipalvelua käyttäen.


## Tavoitteet

- helppokäyttöisyys
- tositteiden sähköinen käsittely pdf-muodossa
- sähköisen arkiston muodostaminen
- sisäänrakennettu laskutus
- muodostaan tuloslaskelman, taseen, tase-erittelyn

## Vaatimukset
Kitupiikki käyttää [Qt-kirjastoa](https://qt.io) versio vähintään 5.10 (Kitsas 5.12)
Pdf-tiedostojen näyttämiseen käytetään [Poppler-kirjastoa](https://poppler.freedesktop.org/) ja zip-tiedostojen käsittelyyn [libzip](https://libzip.org)-kirjastoa.

Lataa ja asenna Qt-kirjastot osoitteesta https://qt.io/download.

Linuxissa poppler on helppo asentaa järjestelmään:

    sudo apt-get install libpoppler-qt5-1 libpoppler-qt5-dev

ja libzip

    sudo apt-get install libzip-dev

## Kääntäminen

Kitupiikki käyttää QMakea. Kääntäminen on helpointa tehdä [QtCreatorin](http://doc.qt.io/qtcreator/) ympäristössä. Komentorivillä kääntyy komennoilla

    qmake kitupiikki.pro && make qmake_all
    make

Kitupiikin Windows-jakeluversion käännetään [MXE-ristiinkääntöympäristössä](https://mxe.cc).

## Kehittäminen

Kehittämisen suuntaviivat löytyvät projektin GitHubin Issues- ja Wiki-osastoista. Koodi kommentoidaan doxygenin merkkauksella niin, että API-dokumentaatio on laadittavissa Doxygenillä. 

## Ylläpitäjä

Arto Hyvättinen <arto@kitsas.fi>

## Lisenssi

GNU General Public License 3 - katso [LICENSE](https://raw.githubusercontent.com/artoh/kitupiikki/master/LICENSE)

Muutettua ohjelmaa levitettäessä tulee ohjelma merkitä selkeästi muutetuksi.
