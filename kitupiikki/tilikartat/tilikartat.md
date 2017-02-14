Tilikartat 
==========

Tämä hakemisto sisältää tilikarttojen pohjat. Ohjelman vakiotilikartat liitetään ohjelmatiedostoon resursseina (tilikartat.qrc) mutta tilikartan voi ladata myös .kpk-tiedostosta.

[TOC]

Tilikarttatiedoston muoto
-------------------------

Tiedosto koostuu osista, joissa on ensin otsikko hakasulkujen välissä

    [nimi]
    Luettelossa näytettävä nimi
 

 
Otsikko                     | Muoto             | Sisältö
----------------------------|-------------------|---------------------------------------
nimi                        | Merkkijono        | Luettelossa näytettävä nimi
kuvaus                      | Tekstiä (html)    | Luettelon vieressä näytettävä tilikartan kuvaus. Kuvauksen pitäisi olla niin tarkka, että sen perusteella voi valita oikean tilikartan
tilit                       | Tilit*(#tilit)    | Tilit
 
 

Tilit
-----

Jokainen tili on omalla rivillään. Ensimmäisessä sarakkeessa on tyyppi ja valinnat, toisessa tilinumero ja kolmannessa nimi.

    H1          10      Vastaavaa
    A           1001    Perustaminen
    AR          1100    Rahavarat
    C23/1100    3100    Myynti 23%
    
Tilityypit
    
Tunnus          | Selite
----------------|--------------------------------
A               | Vastaavaa
AR              | Rahavarojen tili
AP              | Poistettava omaisuus
AL              | Alv-saatavat
B               | Vastattavaa
BE              | Edellisten tilikausien yli/alijäämä
BL              | Alv-velka
C               | Tulot
C**nn**         | Arvonlisäverollinen myynti
D               | Menot
DP              | Poistot

