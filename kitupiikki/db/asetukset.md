Asetukset
=========

Avain	|  Tyyppi		|  Selitys
-------------|------------------|----------------------
Tilinavaus	| int			| Onko tilit jo avattu? <br>0: Tilejä ei avata <br>1: Tilit on avattu <br>2: Tilejä ei ole vielä avattu
TilinavausPvm | QDate 	| Päivämäärä, jolle tilinavaus kirjataan
TilitPaatetty 	| QDate	| Viimeisen päätetyn tilikauden viimeinen päivämäärä. Vain tämän päivän jälkeisille päiville voi tehdä kirjauksia.
Nimi	| QString		| Organisaation nimi
Ytunnus	| QString		| Organisaation y-tunnus
Harjoitus	| bool		| Onko kirjanpito harjoitustilassa
AlvVelvollinen | bool | Onko organisaatio hakeutunut alv-velvolliseksi
AlvVelkatili | int | Tili, jolle arvonlisäverovelka kirjataan
AlvSaatavatili | int | Tili, jolle arvonlisäverosaatavat (palautettava alv) kirjataan
Raportti/(raportin nimi) | QString | Raportin kaava
EkaTositeKirjattu | bool | Käyttäjä on kirjannut tositteen, jonka jälkeen alkuvinkkiä ei enää näytetä
