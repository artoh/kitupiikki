# Monimuotoiset tilikartat

!!! note "Todella edistynyt toiminto"
    Tätä toimintoa tarvitaan vain tilikarttojen tekemiseen.

Kitupiikin elinkeinotoiminnan tilikartoissa voidaan valita [perusvalinnoissa](../perusvalinnat) yritysmuoto. Tällainen monimuotoinen tilikartta on toteutettu joukolla tilikartan asetuksia.

```
[MuotoTeksti]
Yritysmuoto
[MuotoOn/Julkinen osakeyhtiö]
+201..207
+226
+149
[MuotoPois/Julkinen osakeyhtiö]
-201..207
-226
-149
[MuotoOn/Osakeyhtiö]
+201..207
+226
[MuotoPois/Osakeyhtiö]
-201..207
-226
[MuotoOn/Osuuskunta]
+205..210
+227
[MuotoPois/Osuuskunta]
-205..210
-227
[MuotoOn/Kommandiittiyhtiö]
+215..219
+231..235
[MuotoPois/Kommandiittiyhtiö]
-215..219
-231..235
[MuotoOn/Toiminimi]
+220
+236
Elinkeinonharjoittaja=ON
[MuotoPois/Toiminimi]
-220
-236
Elinkeinonharjoittaja=EI
```

**MuotoTeksti**-asetus määrittelee, mikä otsikko näytetään muodon valinnassa. **MuotoOn/<Nimi>** määrittelee komentojonon, joka suoritetaan muotoa valittaessa ja **MuotoPois/<Nimi>** kun valinta poistuu. Aina tilikarttaa päivitettäessä suoritetaan ensin vanhan tilikartan **MuotoPois**-komentojono ja päivityksen lopuksi uuden tilikartan **MuotoOn**-komentojono.

Komentojono voi sisältää seuraavia komentoja

Joukko tilejä voidaan piilottaa, näyttää tai asettaa suosikkitileiksi. Tilit näytetään komennolla `+201..207` missä **+**-merkkiä seuraa tilinumeroväli samalla tavalla kuin raporteissa. Tilit piilotetaan vastaavasti komennolla `-201..207` tai asetetaan suosikiksi `*220`.

Tilien [json-määreitä](https://github.com/artoh/kitupiikki/wiki/Json#tili) voidaan muuttaa komennolla `30..31 AlvProsentti=14` (esimerkki muuttaa tilien 3000-3199 alv-prosentiksi 14).

Kirjanpidon [asetuksia](https://github.com/artoh/kitupiikki/wiki/Asetukset) voidaan muuttaa komennolla `Elinkeinonharjoittaja=ON`. Listamuotoiseen asetukseen voidaan lisätä rivi komennolla `ArkistoRaportit+=Toiminnanalat` ja vastaavasti poistaa komennolla `ArkistoRaportit-=Toiminnanalat`.

## DevTool

!!! danger "Tiedä, mitä teet"
    Ole erittäin huolellinen! DevTool mahdollistaa kaikkien kirjanpidon asetusten suoran muokkaamisen. Varmuuskopiointi ennen DevTool-toimintoja ei ole mitenkään huono idea.

Kun [perusvalintojen](../perusvalinnat) kohta **Näytä tulosteiden muokkauksen työkalut** on valittuna, löytyy **Tilikartan ohje**-määritysvälilehdeltä painike **DevTool**. Painikkeesta avautuu kehittäjille tarkoitettu erikoistyökalu.

![](devtool.png)

**Asetukset**-välilehdellä voit muokata kaikkia kirjanpidon asetuksia ja siten muokata monimuotoisen tilikartan toimintoja.

**Skripti**-välilehdellä voit suorittaa yllä kuvattuja komentojonoja. Niillä voit muokata kerralla suurempaakin tiliryhmää.
