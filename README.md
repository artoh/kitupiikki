# Kitupiikki

**Finnish bookkeeping software for small organisations**

Comments, variable names, documentations and the software, of course, are in Finnish only!


**Suomalainen avoimen lähdekoodin kirjanpito-ohjelma**

Kotisivu ja käyttöohjeet [https:/artoh.github.io/kitupiikki](https://artoh.github.io/kitupiikki)

![Kitupiikki](https://raw.githubusercontent.com/artoh/kitupiikki/master/kitupiikki/pic/aboutpossu.png)

# Tavoitteet

- helppokäyttöisyys
- tositteiden sähköinen käsittely pdf-muodossa
- sähköisen arkiston muodostaminen
- sisäänrakennettu laskutus
- muodostaan tuloslaskelman, taseen, tase-erittelyn

# Vaatimukset
Kitupiikki käyttää [https://qt.io](Qt-kirjastoa) versio vähintään 5.7 sekä [https://poppler.freedesktop.org/](Poppler-kirjastoa) pdf-tiedostojen näyttämiseen.

Lataa ja asenna Qt-kirjastot osoitteesta [https://qt.io/download].

Linuxissa poppler on helppo asentaa järjestelmään:
    
    sudo apt-get install libpoppler-qt5-1 libpoppler-qt5-dev

Windowsissa Kitupiikki käyttää valmiiksi käännettyä Poppler-kirjastoa [https://sourceforge.net/projects/poppler-win32/].

# Kääntäminen

Kitupiikki käyttää QMakea. Kääntäminen on helpointa tehdä [QtCreatorin](http://doc.qt.io/qtcreator/) ympäristössä. Komentorivillä kääntyy komennoilla

    qmake -o Makefile kitupiikki.pro
    make all
    
# Kehittäminen

Kehittämisen suuntaviivat löytyvät projektin GitHubin Issues- ja Wiki-osastoista. Koodi kommentoidaan doxygenin merkkauksella niin, että API-dokumentaatio on laadittavissa Doxygenillä. Kirjanpito talletetaan SQLite-tietokantaan ja liitteet säilytetään pdf-muodossa.

# Tekijä

Arto Hyvättinen

# Lisenssi

GNU General Public License 3 - katso [LICENSE]


