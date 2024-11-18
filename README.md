# Kitsas
Ykkösversio julkaistu nimellä [Kitupiikki](https://kitupiikki.info)

![Kitsas](https://raw.githubusercontent.com/artoh/kitupiikki/master/kitsas/pic/kitsas150.png)

[![versio](https://img.shields.io/github/release/artoh/kitupiikki.svg?label=Julkaistu%20versio)](https://github.com/artoh/kitupiikki/releases)
[![versio](https://img.shields.io/github/release/artoh/kitupiikki/all.svg?label=Esiversio)](https://github.com/artoh/kitupiikki/releases)

**Finnish bookkeeping software for small organisations**

Comments, variable names, documentations and the software itself are, of course, in Finnish only!

**Suomalainen avoimen lähdekoodin kirjanpito-ohjelma**

Kotisivu [kitsas.fi](https://kitsas.fi)   
Käyttöohjeet [kitsas.fi/docs](https://kitsas.fi/docs)


## Tavoitteet

- helppokäyttöisyys
- tositteiden sähköinen käsittely pdf-muodossa
- sähköisen arkiston muodostaminen
- sisäänrakennettu laskutus
- muodostaan tuloslaskelman, taseen, tase-erittelyn

Kirjanpito on mahdollista tallentaa joko omalle tietokoneelle SQLite-muodossa, tai käyttää Kitsas Oy:n palvelinta (maksullinen palvelu), jolloin käytettävissä on myös suuri joukko lisätoimintoja.

## Vaatimukset
Kitsas käyttää [Qt-kirjastoa](https://qt.io) versio vähintään 6.4 (Kaikki ominaisuudet 6.8). Käytössä on mm. QtWidgets, QtPdf ja QtWebEngine -moduulit.

Zip-tiedostojen käsittelyyn käytetään [libzip](https://libzip.org)-kirjastoa.

Lataa ja asenna Qt-kirjastot osoitteesta https://qt.io/download.

Linuxissa poppler on helppo asentaa järjestelmään:

    sudo apt-get install libpoppler-qt5-1 libpoppler-qt5-dev

ja libzip

    sudo apt-get install libzip-dev

## Kääntäminen

Kitsas käyttää QMakea. Kääntäminen on helpointa tehdä [QtCreatorin](http://doc.qt.io/qtcreator/) ympäristössä. Komentorivillä kääntyy komennoilla

    qmake kitupiikki.pro && make qmake_all
    make


## Ylläpitäjä

Arto Hyvättinen <arto@kitsas.fi>

## Lisenssi

GNU General Public License 3 - katso [LICENSE](https://raw.githubusercontent.com/artoh/kitupiikki/master/LICENSE) seuraavilla lisenssin mukaisilla lisäehdoilla:

Jaettaessa muokatuksi ohjelmisto on

-  ohjelmisto merkittävä selkeästi muutetuksi
-  esitettävä selkeästi, ettei Kitsas Oy tarjoa mitään tukea muokatulle ohjelmistolle
- vältettävä käytettämästä Kitsas Oy:n nimeä muokatun ohjelmiston yhteydessä
