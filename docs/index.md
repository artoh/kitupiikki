# Kitupiikki <small>Avoimen lähdekoodin kirjanpitäjä</small>

<div class="karuselli">
 <div class="karusellissa fade">
  <img src="karuselli/1pdf.png">
  <div class="txt">Kitupiikki tallentaa pdf-muotoiset liitteet</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/2paakirja.png">
  <div class="txt">Pääkirja</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/3laskut.png">
  <div class="txt">Kitupiikkiin sisältyy myös laskutus</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/4arkisto.png">
  <div class="txt">Kirjanpidosta muodostetaan sähköinen arkisto</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/5raporttimuokkaus.png">
  <div class="txt">Tulosteet ovat täysin muokattavissa</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/6vinkki.png">
  <div class="txt">Kitupiikki vinkkaa ja neuvoo</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/7viivakoodi.png">
  <div class="txt">Laskuissa pankkiviivakoodi ja QR-koodi</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/8kohdennukset.png">
  <div class="txt">Kirjauksia voi kohdentaa toiminnoille, projekteille jne.</div>
 </div>
 <div class="karusellissa fade">
  <img src="karuselli/tilinpaatos.png">
  <div class="txt">Kitupiikillä muodostat tilinpäätöksen näppärästi</div>
 </div>
 <div class="karusellissa fade">
   <img src="karuselli/9poistot.png">
   <div class="txt">Kitupiikki laskee tasaerä- ja menojäännöspoistot</div>
 </div>
 <div class="karusellissa fade">
   <img src="karuselli/alv.png">
   <div class="txt">Kitupiikki laskee alv-ilmoituksen tiedot</div>
 </div>
 <div class="karusellissa fade">
   <img src="karuselli/taseerittely.png">
   <div class="txt">Kitupiikki muodostaa tase-erittelyn</div>
 </div>
 <div class="karusellissa fade">
   <img src="karuselli/selaus.png">
   <div class="txt">Kirjausten selaaminen</div>
 </div>  
 <div class="karusellissa fade">
   <img src="karuselli/budjetti.png">
   <div class="txt">Kitupiikissä on budjetin laadinta ja seuranta</div>
 </div>   
 <div class="karusellissa fade">
   <img src="karuselli/raportit.png">
   <div class="txt">Kitupiikissä on monipuolinen raportointi</div>
 </div>   
 <div class="karusellissa fade">
   <img src="karuselli/mac.png">
   <div class="txt">Saatavilla Windowsille, Macintoshille ja Linuxille</div>
 </div>     
</div>

<p class="intro">Kitupiikki on ilmainen suomalainen kirjanpito-ohjelma. Kitupiikki käsittelee ja arkistoi skannatut tositteet. Ohjelma on alunperin suunniteltu yhdistyksille, mutta Kitupiikin avulla hoidat vaivattomasti myös pienehkön yrityksen kirjanpidon.</p>

<div class="asennusinfo">
<h3>Kitupiikki 1.4.1 julkaistu 22.8.2019</h3>


<div class="asennuslaatikko">
<a href="asennus" style="color:white;"><span class="fa fa-download"></span> Lataa Kitupiikki <span class="fa fa-windows"></span> <span class="fa fa-linux"></span>  <span class="fa fa-apple"></span></a>
</div>

</div>

<script>
var slideIndex = Math.floor( Math.random() * document.getElementsByClassName("karusellissa").length );
showSlides();

function showSlides() {
    var i;
    var slides = document.getElementsByClassName("karusellissa");
    for (i = 0; i < slides.length; i++) {
        slides[i].style.display = "none";
    }

    slideIndex = slideIndex + 1;
    if( slideIndex >= slides.length)
      slideIndex = 0;

    slides[ slideIndex ].style.display = "block";
    setTimeout(showSlides, 5000); // Kuva vaihtuu muutaman sekunnin välein
}

</script>

<!-- Googlen jäsenneltyjen tietojen merkintäapurin luomat JSON-LD-merkinnät. -->
<script type="application/ld+json">
{
"@context" : "http://schema.org",
"@type" : "SoftwareApplication",
"name" : "Kitupiikki",
"image" : "https://kitupiikki.info/images/kitupiikkikannettava.png",
"url" : "https://kitupiikki.info",
"author" : {
"@type" : "Person",
"name" : "Arto Hyvättinen"
},
"downloadUrl" : "https://lataa.kitupiikki.info",
"operatingSystem" : "Windows,Linux,macOS",
"screenshot" : "https://kitupiikki.info/myllykirjaus.png",
"applicationCategory": "Office",
"applicationSubCategory": "Finance",
"inLanguage" : "fi",
"license" : "https://www.gnu.org/licenses/gpl-3.0.en.html",
"offers": {
"@type": "Offer",
"priceCurrency": "EUR",
"price": "0.00"
}

}

</script>
