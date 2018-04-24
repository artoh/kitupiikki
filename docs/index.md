# Kitupiikki <small>Avoimen lähdekoodin kirjanpitäjä</small>

<div class="karuselli">
<div class="karusellissa fade">
 <img src="images/kitupiikkikannettava.png">
  <div class="txt">Avoimen lähdekoodin kirjanpitäjä</div>
</div>
 <div class="karusellissa fade">
  <img src="kirjaus/myllykirjaus.png">
  <div class="txt">Kitupiikki tallentaa pdf-muotoiset liitteet</div>
 </div>
 <div class="karusellissa fade">
  <img src="images/paakirja.png">
  <div class="txt">Pääkirja</div>
 </div>
 <div class="karusellissa fade">
  <img src="laskutus/luettelo.png">
  <div class="txt">Kitupiikkiin sisältyy myös laskutus</div>
 </div>
 <div class="karusellissa fade">
  <img src="images/maksualv.png">
  <div class="txt">Kitupiikki tukee maksuperusteista arvonlisäveroa</div>
 </div>
 <div class="karusellissa fade">
  <img src="images/arkisto.png">
  <div class="txt">Kirjanpidosta muodostetaan sähköinen arkisto</div>
 </div>
 <div class="karusellissa fade">
  <img src="images/raportinmuokkaus.png">
  <div class="txt">Tulosteet ovat täysin muokattavissa</div>
 </div>
 <div class="karusellissa fade">
  <img src="aloitus/vinkit7.png">
  <div class="txt">Kitupiikki vinkkaa ja neuvoo</div>
 </div>
 <div class="karusellissa fade">
  <img src="images/viivakoodi.png">
  <div class="txt">Laskuissa pankkiviivakoodi ja QR-koodi</div>
 </div>
 <div class="karusellissa fade">
  <img src="images/arkisto1.png">
  <div class="txt">Sähköisen arkiston etusivu</div>
 </div>
 <div class="karusellissa fade">
  <img src="images/arkisto1.png">
  <div class="txt">Sähköisen arkiston etusivu</div>
 </div>
 <div class="karusellissa fade">
  <img src="maaritykset/kohdennukset/kohdennukset.png">
  <div class="txt">Kirjauksia voi kohdentaa toiminnoille, projekteille jne.</div>
 </div>
</div>


Kitupiikki on ilmainen suomalainen kirjanpito-ohjelma. Kitupiikki käsittelee ja arkistoi skannatut tositteet. Ohjelma on alunperin suunniteltu yhdistyksille, mutta Kitupiikin avulla hoidat vaivattomasti myös pienehkön yrityksen kirjanpidon.

<div class="asennusinfo">
<h3>Kitupiikki 0.10.1 (beta) julkaistu 17.4.2018</h3>
Uutena kohdennusmahdollisuus tasetileillä ja kokeellinen TITO-tiliotteiden tuominen

<div class="asennuslaatikko">
<a href="asennus" style="color:white;"><span class="fa fa-download"></span> Lataa Kitupiikki <span class="fa fa-windows"></span> <span class="fa fa-linux"></span></a>
</div>

</div>

<script>
var slideIndex = 0;
showSlides();

function showSlides() {
    var i;
    var slides = document.getElementsByClassName("karusellissa");
    for (i = 0; i < slides.length; i++) {
        slides[i].style.display = "none";
    }
    slides[ Math.floor( Math.random() * slides.length )].style.display = "block";
    setTimeout(showSlides, 5000); // Kuva vaihtuu kahden sekunnin välein
}

</script>
