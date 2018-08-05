# Kitupiikki

[![versio](https://img.shields.io/github/release/artoh/kitupiikki.svg?label=Julkaistu%20versio)](https://github.com/artoh/kitupiikki/releases)
[![versio](https://img.shields.io/github/release/artoh/kitupiikki/all.svg?label=Esiversio)](https://github.com/artoh/kitupiikki/releases)
[![lisenssi](https://img.shields.io/github/license/artoh/kitupiikki.svg?label=Lisenssi)](https://raw.githubusercontent.com/artoh/kitupiikki/master/LICENSE)
[![mathdown HuBoard](https://img.shields.io/github/issues/artoh/kitupiikki.svg?label=Tehtävät%20%28HuBoard%29)](https://huboard.com/artoh/kitupiikki)
[![Join the chat at https://gitter.im/kitupiikki/Lobby](https://badges.gitter.im/kitupiikki/Lobby.svg)](https://gitter.im/kitupiikki/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**Finnish bookkeeping software for small organisations**

Comments, variable names, documentations and the software itself are, of course, in Finnish only!


**Suomalainen avoimen lähdekoodin kirjanpito-ohjelma**

Kotisivu ja käyttöohjeet [https://kitupiikki.info](https://kitupiikki.info)

![Kitupiikki](https://raw.githubusercontent.com/artoh/kitupiikki/master/kitupiikki/pic/aboutpossu.png)

## Tavoitteet

- helppokäyttöisyys
- tositteiden sähköinen käsittely pdf-muodossa
- sähköisen arkiston muodostaminen
- sisäänrakennettu laskutus
- muodostaan tuloslaskelman, taseen, tase-erittelyn

## Vaatimukset
Kitupiikki käyttää [Qt-kirjastoa](https://qt.io) versio vähintään 5.10.
Pdf-tiedostojen näyttämiseen käytetään [Poppler-kirjastoa](https://poppler.freedesktop.org/).

Lataa ja asenna Qt-kirjastot osoitteesta https://qt.io/download.

Linuxissa poppler on helppo asentaa järjestelmään:

    sudo apt-get install libpoppler-qt5-1 libpoppler-qt5-dev

## Kääntäminen

Kitupiikki käyttää QMakea. Kääntäminen on helpointa tehdä [QtCreatorin](http://doc.qt.io/qtcreator/) ympäristössä. Komentorivillä kääntyy komennoilla

    qmake kitupiikki.pro && make qmake_all
    make

Kitupiikin Windows-jakeluversion käännetään [MXE-ristiinkääntöympäristössä](https://mxe.cc).

## Kehittäminen

Kehittämisen suuntaviivat löytyvät projektin GitHubin Issues- ja Wiki-osastoista. Koodi kommentoidaan doxygenin merkkauksella niin, että API-dokumentaatio on laadittavissa Doxygenillä. Kirjanpito talletetaan SQLite-tietokantaan ja liitteet säilytetään pdf-muodossa.

## Ylläpitäjä

Arto Hyvättinen <devel@kitupiikki.info>

## Lisenssi

GNU General Public License 3 - katso [LICENSE](https://raw.githubusercontent.com/artoh/kitupiikki/master/LICENSE)
