# Kirjaaminen käsin


!!! note "Käytä Kirjausapuria aina kuin voit"
    Kirjauksiin liittyy usein paljon erilaisia ohjaustietoja, kuten arvonlisäveron ja tase-erien tiedot. Nämä tiedot tulevat kaikkein varmimmiten oikein käyttämällä Kirjausapuria. Käytä siksi Kirjausapuria aina kuin se on mahdollista!

Voit tehdä kirjauksen myös ilman Kirjausapuria valitsemalla **Viennit**-välilehdellä **Lisää rivi** (tai painamalla <kbd>F11</kbd>).

![](ruudukko.png)

Sarakkeeseen **Tili** voit syöttää tilinumeron tai kirjoittaa tilin nimen alkua. Jos kirjaat tasetilille, jossa on käytettävissä tase-erittely, pääset kohdennus-saraketta napsauttamalla valitsemaan kirjaukselle tase-erän.

## Arvonlisäveron kirjaaminen

Arvonlisäveron tietoja pääset muokkaamaan napsauttamalla Alv-saraketta, josta avautuu valintaikkuna. Tehtäessä arvonlisäverollisia kirjauksia pitää vientiin määritellä aina oikeat arvonlisäveron ohjaustiedot.

Katso tarkemmin luvusta [Arvonlisävero](/alv) eri arvonlisäveron kirjaustyypit.

![](/alv/alvvalinta.png)

Kotimaan arvonlisäverot kirjataan myyntien osalta seuraavasti

Verolaji |  Kirjaamisohje
--------------|---------------------
Verollinen myynti (netto)  | Myynti-tilille kirjataan veroton määrä alv-tiedolla *Veronalainen määrä* ja *Verokanta*. Alv-velkatilille kirjataan veron osuus alv-tiedolla *Veron määrä* ja *Verokanta*
Verollinen myynti (brutto) | Koko bruttohinta kirjataan myynti-tilille alv-tiedolla *Veronalainen määrä* ja *Verokanta*. Vero erotellaan tästä kirjauksesta alv-ilmoituksen yhteydessä.
Verollinen myynti (maksuperusteinen alv) | Myynti-tilille kirjataan veroton määrä alv-tiedoilla *Veronalainen määrä* ja *Verokanta*. Maksuperusteisen alv-velan tilille kirjataan alv:n määrä tiedoilla *Kohdentamaton maksuperusteinen alv* ja *Verokanta*.

Muut arvonlisäverot kirjataan aina nettoperusteisesti niin, että alv-tiedolla *Veronalainen määrä* merkitään veron peruste ja alv-tiedolla *Veron määrä* maksettava vero ja vastaavasti *Vähennettävä määrä* alv-vähennykset määrä.

## Kohdennuksen kirjaaminen

Tileillä, joissa ei ole käytössä tase-erittelyä, voi kohdennuksen valita suoraan **Kohdennus**-sarakkeelta avautustava valintalistasta. Tase-eritellyillä tileillä kohdennus valitaan valintaikkunasta, joka avautuu napsauttamalla **Kohdennus**-saraketta. **Merkkaukset** valitaan sarakkeen päällä hiiren oikeasta painikkeesta avautuvasta valikosta.


### Osto- ja myyntilaskujen lisätiedot

![](olkohdennus.png)

Kirjattaessa **Ostovelat**-tilille on kohdennusikkunassa käytettävissä myös ostolaskun lisätiedot. Näitä tietoja käytetään kohdennettaessa tiliotteelta tuotava suoritus oikealle laskulle. Kitupiikki pyrkii lisäämään nämä tiedot automaattisesti pdf-muotoiselta laskulta.

Vastaavasti **Myyntisaatavat**-tilille kirjattaessa voit antaa viitetiedot, joiden perusteella saapuva maksu kohdennetaan.

Kun ostolaskulla on kohdennustietoja, näkyy Kohdennus-sarakkeessa sana **VIITE**.
