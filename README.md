# Kitsas
Ykkösversio julkaistu nimellä [Kitupiikki](https://kitupiikki.info)

![Kitsas](https://raw.githubusercontent.com/artoh/kitupiikki/master/kitsas/pic/kitsas150.png)

[![versio](https://img.shields.io/github/release/artoh/kitupiikki.svg?label=Julkaistu%20versio)](https://github.com/artoh/kitupiikki/releases)
[![versio](https://img.shields.io/github/release/artoh/kitupiikki/all.svg?label=Esiversio)](https://github.com/artoh/kitupiikki/releases)
[![mathdown HuBoard](https://img.shields.io/github/issues/artoh/kitupiikki.svg?label=Tehtävät%20%28HuBoard%29)](https://huboard.com/artoh/kitupiikki)

**Finnish bookkeeping software for small organisations**

Comments, variable names, documentations and the software itself are, of course, in Finnish only!

**Suomalainen avoimen lähdekoodin kirjanpito-ohjelma**

Kotisivu [kitsas.fi](https://kitsas.fi)   
Käyttöohjeet [kitsas.fi/docs](https://kitsas.fi.docs)


## Tavoitteet

- helppokäyttöisyys
- tositteiden sähköinen käsittely pdf-muodossa
- sähköisen arkiston muodostaminen
- sisäänrakennettu laskutus
- muodostaan tuloslaskelman, taseen, tase-erittelyn

## Vaatimukset
Kitsas käyttää [Qt-kirjastoa](https://qt.io) versio vähintään 5.15 tai 6.4

Pdf-tiedostojen näyttämiseen käytetään [Poppler-kirjastoa](https://poppler.freedesktop.org/) ja zip-tiedostojen käsittelyyn [libzip](https://libzip.org)-kirjastoa.

Lataa ja asenna Qt-kirjastot osoitteesta https://qt.io/download.

Linuxissa poppler on helppo asentaa järjestelmään:

    sudo apt-get install libpoppler-qt5-1 libpoppler-qt5-dev

ja libzip

    sudo apt-get install libzip-dev

## Kääntäminen

Kitsas käyttää QMakea. Kääntäminen on helpointa tehdä [QtCreatorin](http://doc.qt.io/qtcreator/) ympäristössä. Komentorivillä kääntyy komennoilla

    qmake kitupiikki.pro && make qmake_all
    make

Kitsaan Windows-jakeluversion käännetään [MXE-ristiinkääntöympäristössä](https://mxe.cc).


## Ylläpitäjä

Arto Hyvättinen <arto@kitsas.fi>

## Lisenssi

GNU General Public License 3 - katso [LICENSE](https://raw.githubusercontent.com/artoh/kitupiikki/master/LICENSE) seuraavilla lisenssin mukaisilla lisäehdoilla:

Jaettaessa muokatuksi ohjelmisto on

-  ohjelmisto merkittävä selkeästi muutetuksi
-  esitettävä selkeästi, ettei Kitsas Oy tarjoa mitään tukea muokatulle ohjelmistolle
- vältettävä käytettämästä Kitsas Oy:n nimeä muokatun ohjelmiston yhteydessä
