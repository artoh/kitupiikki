# Tilinpäätöksen malli

!!! note "Edistynyt toiminto"
    Sinun ei yleensä tarvitse muokata raportteja. Raporttien muokkaus on otettava erikseen käyttöön [perusvalintojen](../perusvalinnat) kohdasta **Näytä tulosteiden muokkauksen työkalut**

![](malli.png)

Tässä määritellään tilinpäätökseen tulostuvat raportit sekä liitetietojen kaava.

Erikoismerkeillä alkavat värilliset rivit määrittelevät ehtoja ja tulosteita, ja kaikki muu html-muotoinen teksti tulostuu tilinpäätöksen liitetietoihin.

Seuraavat rivit määrittelevät, että niiden jälkeen tulostuu vain tietyn kokoluokan yrityksille

* **#MIKRO** Mikroyrityksille
* **#PIEN** Pienyrityksille
* **#ISO** Yrityksille, jotka eivät ole mikro- eivätkä pienyrityksiä
* **#HENKILOSTO** Yrityksille, joilla on henkilöstöä

**##** tarkoittaa, että rivi on tilinpäätöksen muodostamisen valintaikkunaan tulostuva otsikko

![](muodostaminen.png)

**#**-alkuiset rivit määrittelevät tulostusehtoja. Ne ovat muotoa

`#nimi -pois -P Valintaikkunaan tulostuva teksti`

* `nimi` on valinnan tunnus
* `pois` on sellaisen valinnan tunnus, joka ei voi olla samanaikaisesti valittuna
* `-P` tarkoittaa, ettei tätä valintaa edes näytetä pienyrityksille (vastaavasti `-M` ei näytetä mikroyrityksille eikä `-I` pienyritystä isommille yrityksille)
* Loppurivillä on valintaikkunaan tulostuvaa tekstiä

Rivi voi olla myös ilman tulostuvaa tekstiä, jos valinnan tunnus on määritelty jo aiemmin. Jos rivillä on pelkästään **-**-merkillä alkavia poissulkevia tunnuksia, tulee rivi valituksi, ellei yksikään mainittu tunnus ole valittu.

Pelkkä **#** lopettaa ehdon, jonka jälkeiset rivit tulostuvat kaikilla valinnoilla.

**@**-alkuiset rivit määrittelevät erityisen tulostettavan

* `@Raportin nimi!Otsikko@` määrittelee, että mainittu raportti liitetään tilinpäätökseen, ja `@Raportin nimi*Otsikko@` vastaavasti raportin erittelylle (nämä voi liittää helpommin **Lisää raportti**-napilla)
* `@henkilosto@` lisää taulukon henkilöstön määrästä tällä ja edellisellä tilikaudella
* `@sha@` lisää arkiston sha256-tiivisteen
