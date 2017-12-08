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
Raportti/(raportin nimi) | QString | Raportin kaava
EkaTositeKirjattu | bool | Käyttäjä on kirjannut tositteen, jonka jälkeen alkuvinkkiä ei enää näytetä
NaytaEdistyneet | bool | Näytetäänkö muokkaimet
Osoite | QString | Tulostettava osoite
Kotipaikka | QString | Organisaation kotipaikka
AlvIlmoitus | QDate |   Viimeisin alv-ilmoituksen pvm
AlvKausi  | int | Kausi kuukautta
LaskuTositelaji | int | Tositelaji, jolle laskut kirjataan
LaskuKirjausperuste | int | Oletus laskun kirjausperusteelle (LaskuModel)
LaskuSaatavatili | int | Tilinumero, jolle laskun saatava kirjataan
LaskuKateistili | int | Tilinumero, jolle käteislasku kirjataan
LaskuMaksuaika | int | Oletusmaksuaika vrk
LaskuHuomautusaika | QString | tulostettava huomautusaika
LaskuViivastyskorko | QString | tulostettava viivästyskorkoteksti
LaskuSeuraavaId | int | seuraavan laskun laskunro
LaskuNaytaTuotteet | bool | näytetäänkö tuotteet
IBAN | QString | laskulle tulostettava IBAN
Puhelin | QString | laskulle tulostettava puhelinnumero
EmailNimi | QString | sähköpostien lähettäjänimi
EmailOsoite | QString | sähköpostien lähettäjäosoite
