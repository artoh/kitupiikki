Json-kentät
===========

Tili
---

Avain	| Tyyppi 	|  Selitys
-------------|------------|------------------
Vastatili	| int		| Oletusvastatilin tilinumero
AlvLaji	| int		| Oletus viennin verokohtelulle
AlvProsentti| int	| Oletus viennin veroprosentille
Taydentava | str | Tilin täydentävä nimike
Kirjausohje | str | Tilin kirjausohje
Tasaerapoisto | int | Oletus tasaeräpoiston aika kuukautta
Menojaannospoisto | int | Tilin menojäännöspoisto %
Taseerittely | int | Miten tase-erittely laaditaan: **0** ei erittelyä, **1** saldot, **2** muutokset, **3** seurataan eriä

Tositelaji
-----

Avain   | Tyyppi  |  Selitys
--------|---------|-----------------
Vastatili | int (tilinumero) | Oletus kirjausten vastatilille
Kirjaustyyppi | int (enum TositelajiModel::KirjausTyyppi) | Tositteen kirjausten tyyppi: KAIKKIKIRJAUKSET, OSTOLASKUT, MYYNTILASKUT, TILIOTE
Oletustili  |  int(tilinumero)  | Oletus kirjaustiliksi

Tosite
------

Avain   | Tyyppi  |  Selitys
--------|---------|-----------------
TilioteAlkaa | QDate | mistä tiliote alkaa
TilioteLoppuu | QDate | mihin tiliote TilioteLoppuu
Lasku | int | Tosite on lasku, laskun nro

