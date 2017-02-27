Json-kentät
===========

Tili
---

Avain	| Tyyppi 	|  Selitys
-------------|------------|------------------
Vastatili	| int		| Oletusvastatilin tilinumero
AlvLaji	| int		| Oletus viennin verokohtelulle
AlvProsentti| int	| Oletus viennin veroprosentille

Tositelaji
-----

Avain   | Tyyppi  |  Selitys
--------|---------|-----------------
Vastatili | int (tilinumero) | Oletus kirjausten vastatilille
Kirjaustyyppi | int (enum TositelajiModel::KirjausTyyppi) | Tositteen kirjausten tyyppi: KAIKKIKIRJAUKSET, OSTOLASKUT, MYYNTILASKUT, TILIOTE
Oletustili  |  int(tilinumero)  | Oletus kirjaustiliksi

Tosite
------

- tiliote alkaa
- tiliote päättyy
