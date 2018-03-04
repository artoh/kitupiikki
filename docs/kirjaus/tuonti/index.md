# Tuonti

!!! info "Uusi toiminto"
    Tuontitoiminto on vasta lisätty Kitupiikkiin, eikä sitä ole ollut mahdollista kokeilla kovin monipuolisella aineistolla. Lähetä mieluusti [palautetta](https://form.jotformeu.com/73283959099374) tuonnin toimivuudesta.
    CSV-tuonnin virheiden osalta liitä mukaan pätkä CSV:tä: otsikot ja jokin rivi (voit muuttaa yksityisen tiedon XXXXXXXXX)

## Pdf-tiedostojen tuonti

Kun kirjausikkunassa lisätään pdf-muotoinen tosite, pyrkii Kitupiikki poimimaan siitä tietoja kirjauksien pohjaksi.

Toiminto edellyttää, että pdf-tiedostossa teksti on upotettuna. Näin on yleensä, kun pdf-tiedosto on ollut koko ajan sähköisessä muodossa (välitetty sähköpostilla ja haettu verkkopankista). Skannattu tiedosto pitää käsitellä erikseen OCR- eli tekstintunnistusohjelmalla. Lisäksi pdf-tiedostojen tuonti pitää olla otettuna [määrityksistä](#maaritykset) käyttöön.

### Ostolaskut

!(Ostolasku)[ostolasku.png]

Pdf-ostolaskun pohjalta Kitupiikki tekee valmiiksi **Ostovelat**-tilin kirjauksen. Valitsemalla **Apuri** tai **F9** pääset kirjaamaan Apurilla laskun oikealle menotilille.

Lisäksi ostolaskulle tallennetaan lisätiedot (Saajan pankkitili ja viite), joiden avulla maksu on kohdennettavissa tiliotteelta.

!!! info "Ostolaskujen muoto"
    Kitupiikki pyrkii etsimään laskun tietoja tiettyjen laskulla olevien sanojen avulla. Parhaiten ostolaskun tiedot löytyvät, jos laskun alareunassa on tilisiirtolomake, josta tiedot saa poimittua.

### Tiliotteet

Tiliotteen perusteella Kitupiikki tekee valmiit pankkitilin kirjaukset. Jos tiliotteella olevan tapahtuman IBAN- ja viitenumerot täsmäävät avoinna olevaan myynti- tai ostolaskuun, osaa Kitupiikki kohdentaa suorituksen ja tekee koko kirjauksen valmiiksi. Verohallinnon oma-aloitteisten verojen maksutilille tehdyt suoritukset kohdistetaan verovelkatilille.

Tiliotteen kirjaaminen edellyttää, että tilin IBAN-numero on määritelty kyseisen tilin [määrityksissä](/maaritykset/tilikartta).

!!! info "Tiliotteiden muoto"
    Tiliotteiden tuonti perustuu tiettyihin tiliotteella oleviin sanoihin ja niiden asemointiin. Kitupiikki on onnistunut tunnistamaan ainakin seuraavien pankkien tiliotteita: S-Pankki, XXXXXX.

### Määritykset

![Tuontimääritykset](tuontimaaritys.png)

Määrityksien **Tuonti** välilehdellä pdf-tiedostojen tuonti otetaan käyttöön (erikseen ostolaskujen ja tiliotteiden osalta). Ostolaskuista määritellään myös tuotavien laskujen **tositetyyppi** ja **ostolaskutili** sekä se, tehdäänkö kirjaukset **suoriteperusteisina** (toimituspäivämäärän mukaan, jos mainittu laskulla) vai **laskuperusteisesti** (laskun päivämäärän mukaan).

## CSV-tiedostojen tuonti

CSV-tiedosto on tekstitiedosto, jossa eri tietokentät on eroteltu toisistaan pilkulla. Voit tuoda Kitupiikkiin tällä toiminnolla esimerkiksi [Tilitin-kirjanpito-ohjelman](http://helineva.net/tilitin) kirjauksia tai verkkopankin tilitapahtumia. Kitupiikin kirjauksia voit tuoda csv-muotoisesta [Päiväkirja-raportista](/tulosteet/paivakirja).

Voit tuoda kirjanpitoon csv-tiedoston lisäämällä **.csv**-päätteisen tiedoston kirjaukseen (raahaamalla **Sähköinen tosite**-laatikkoon tai **Lisää tiedosto**-painikkeella). Saat ikkunan, jossa valitaan, mitä tietoja csv-tiedoston eri sarakkeista tuodaan.

![Tilitapahtumien tuonti](tapahtumat.png)

Jos tuot **tilitietoja**, valitse lisäksi tili, jolle tiedot kirjataan. Tilitiedot käsitellään ja kohdennetaan samalla tavalla kuin pdf-tiliotteelta.

**Kirjauksia** tuotaessa saat **Muunna tilinumerot**-valinnalla vielä erillisen valintaikkunan, jossa määrittelet tilinumeroiden muuttamisen tuotaessa käytössäsi olevan tilikartan tileiksi. Vasemmanpuoleisessa sarakkeessa ovat csv-tiedoston tilinumerot ja oikeanpuolimmaisesta voit valita, mille Kitupiikin tilille kirjaus tuodaan.

![Tilinumeroiden muunto](muunto.png)

### CSV-tiedoston muoto

CSV-tiedostossa erottimena saa olla pilkku, puolipiste tai tabulaattori. Päivämäärät voivat olla suomalaisessa (31.12.2018) tai ISO-muodossa (2018-12-31). Tiedostossa pitää olla otsikkorivi, mutta otsikoiden nimet voivat olla mitä tahansa - sarakkeiden sisältö valitaan aina tuotaessa.

Esimerkki tuotavista tilitapahtumista
```

```

Esimerkki tuotavista kirjauksista
```

```
