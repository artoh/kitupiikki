# Kitupiikki


[![versio](https://img.shields.io/github/release/artoh/kitupiikki.svg?label=Julkaistu%20versio)](https://github.com/artoh/kitupiikki/releases)
[![versio](https://img.shields.io/github/release/artoh/kitupiikki/all.svg?label=Esiversio)](https://github.com/artoh/kitupiikki/releases)
![lisenssi](https://img.shields.io/github/license/artoh/kitupiikki.svg?label=Lisenssi)
[![mathdown HuBoard](https://img.shields.io/github/issues/artoh/kitupiikki.svg?label=Tehtävät%20%28HuBoard%29)](https://huboard.com/artoh/kitupiikki)

**Finnish bookkeeping software for small organisations**

Comments, variable names, documentations and the software itself are, of course, in Finnish only!


**Suomalainen avoimen lähdekoodin kirjanpito-ohjelma**

Kotisivu ja käyttöohjeet [https://artoh.github.io/kitupiikki](https://artoh.github.io/kitupiikki)

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

Windowsissa voi käyttää [valmiiksi käännettyä Poppler Win32](https://sourceforge.net/projects/poppler-win32/) -kirjastoa, joka puretaan kitupiikin hakemistoon. Päivitysten tarkistaminen ja SMTP/SSL-sähköpostin lähettäminen vaativat OpenSSL-kirjastoa.


## Kääntäminen

Kitupiikki käyttää QMakea. Kääntäminen on helpointa tehdä [QtCreatorin](http://doc.qt.io/qtcreator/) ympäristössä. Komentorivillä kääntyy komennoilla

    qmake -o Makefile kitupiikki.pro
    make all

## Kehittäminen

Kehittämisen suuntaviivat löytyvät projektin GitHubin Issues- ja Wiki-osastoista. Koodi kommentoidaan doxygenin merkkauksella niin, että API-dokumentaatio on laadittavissa Doxygenillä. Kirjanpito talletetaan SQLite-tietokantaan ja liitteet säilytetään pdf-muodossa.

## Tekijä

Arto Hyvättinen

## Lisenssi

GNU General Public License 3 - katso [LICENSE](https://raw.githubusercontent.com/artoh/kitupiikki/master/LICENSE)
