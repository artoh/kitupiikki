/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "apuririvi.h"
#include "db/verotyyppimodel.h"
#include "model/tositevienti.h"
#include "db/kirjanpito.h"


ApuriRivi::ApuriRivi(int tili)
{
    if( tili ) {
        tilinumero_=tili;
        Tili* tilini = kp()->tilit()->tili(tili);
        bool alv = kp()->asetukset()->onko(AsetusModel::AlvVelvollinen);
        if( tilini ) {
            alvkoodi_ = alv ? tilini->luku("alvlaji") : AlvKoodi::EIALV;
            veroprosentti_ = alv ? tilini->alvprosentti() : 0;
            poistoaika_ = tilini->luku("menojaannospoisto");
            kohdennus_ = tilini->luku("kohdennus");
        }
    }
}

ApuriRivi::ApuriRivi(const TositeVienti &vienti, bool plusOnKredit)
{    

    vientiId_ = vienti.id();
    tilinumero_ = vienti.tili();
    selite_ = vienti.selite();
    alvkoodi_ = vienti.alvKoodi();
    veroprosentti_ = vienti.alvProsentti();
    kohdennus_ = vienti.kohdennus();
    merkkaukset_ = vienti.merkkaukset();
    jaksoalkaa_ = vienti.jaksoalkaa();
    jaksoloppuu_ = vienti.jaksoloppuu();
    poistoaika_ = vienti.tasaerapoisto();
    era_ = vienti.era();
    alvvahennys_ = true;

    // Sitten netto/brutto tilanteen mukaan

    int verokoodi = alvkoodi();
    bool nettoon = verokoodi == AlvKoodi::MYYNNIT_NETTO  || verokoodi == AlvKoodi::OSTOT_NETTO ||
            verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI || verokoodi== AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
            verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi== AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI ||
            verokoodi == AlvKoodi::MAAHANTUONTI_PALVELUT;

    Euro maara = plusOnKredit ?
                 vienti.kreditEuro() - vienti.debetEuro() :
                 vienti.debetEuro() - vienti.kreditEuro();

    if( nettoon )
        setNetto( maara);
    else
        setBrutto( maara );

}

int ApuriRivi::alvkoodi() const
{
    return alvkoodi_;
}

void ApuriRivi::setEra(const EraMap &era)
{
    era_ = era;
}

int ApuriRivi::poistoaika() const
{
    return poistoaika_;
}

void ApuriRivi::setAlvkoodi(int koodi)
{
    if( kp()->asetukset()->onko(AsetusModel::AlvVelvollinen) )
        alvkoodi_ = koodi;
    else
        alvkoodi_ = AlvKoodi::EIALV;
}

void ApuriRivi::setAlvprosentti(double prosentti)
{
    if(kp()->asetukset()->onko(AsetusModel::AlvVelvollinen))
        veroprosentti_ = prosentti;
    else
        veroprosentti_ = 0.0;
}

void ApuriRivi::setAlvvahennys(bool vahennys, int vahennysVientiId)
{
   alvvahennys_ = vahennys;
   if( vahennysVientiId) vahennysVientiId_ = vahennysVientiId;
}

Euro ApuriRivi::brutto() const
{
    if( !brutto_ ) {
        if( kp()->alvTyypit()->nollaTyyppi(alvkoodi_))
            return netto_;
        return Euro::fromCents(qRound64( ( 100 + veroprosentti_) * netto_.cents() / 100.0 ));
    } else
        return brutto_;
}

Euro ApuriRivi::netto() const
{
    if( !netto_ ) {
        if( kp()->alvTyypit()->nollaTyyppi(alvkoodi_) )
            return brutto_;
        Euro vero = Euro::fromCents(qRound64( brutto_.cents() * veroprosentti_ / ( 100 + veroprosentti_) ));
        return brutto_ - vero;
    } else
        return netto_;
}

Euro ApuriRivi::naytettava() const
{
    // Näytetään ensisijaisesti bruttosumma
    if( naytaBrutto())
        return brutto();
    else
        return netto();
}

void ApuriRivi::setBrutto(Euro eurot)
{
    brutto_ = eurot;
    netto_ = 0;
}

void ApuriRivi::setNetto(Euro eurot)
{
    netto_ = eurot;
    brutto_ = 0;
}

void ApuriRivi::setNetonVero(Euro eurot, int vientiId)
{
    brutto_ = netto_ > Euro::Zero ? netto_ + eurot : netto_ - eurot.abs();
    veroVientiId_ = vientiId;
}

bool ApuriRivi::naytaBrutto() const
{
    return !( alvkoodi() == AlvKoodi::RAKENNUSPALVELU_OSTO || alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_TAVARAT
                                 || alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || alvkoodi() == AlvKoodi::MAAHANTUONTI
                                 || alvkoodi() == AlvKoodi::MAAHANTUONTI_VERO || alvkoodi() == AlvKoodi::MAAHANTUONTI_PALVELUT)  ;

}

bool ApuriRivi::naytaNetto() const
{
    return  alvkoodi() == AlvKoodi::OSTOT_NETTO || alvkoodi() == AlvKoodi::MYYNNIT_NETTO ||
                                 alvkoodi() == AlvKoodi::OSTOT_BRUTTO || alvkoodi() == AlvKoodi::MYYNNIT_BRUTTO ||
                                 alvkoodi() == AlvKoodi::MAKSUPERUSTEINEN_OSTO || alvkoodi() == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
            !naytaBrutto() ;
}

bool ApuriRivi::naytaVahennysvalinta() const
{
    return alvkoodi() == AlvKoodi::RAKENNUSPALVELU_OSTO ||
            alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            alvkoodi() == AlvKoodi::YHTEISOHANKINNAT_PALVELUT ||
            alvkoodi() == AlvKoodi::MAAHANTUONTI ||
            alvkoodi() == AlvKoodi::MAAHANTUONTI_VERO ||
            alvkoodi() == AlvKoodi::MAAHANTUONTI_PALVELUT;
}

QVariantList ApuriRivi::viennit(const TositeVienti::VientiTyyppi tyyppi, const bool plusOnKredit,
                                const QString& otsikko, const QVariantMap& kumppani, const QDate& pvm) const
{
    QVariantList vientilista;
    // Ensin varsinainen vienti

    if( !brutto())
        return vientilista;

    Euro maara = this->brutto();
    Euro netto = this->netto();
    Euro vero = this->brutto() - this->netto() ;

    QString rivinSelite = selite().isEmpty() ?
                otsikko :
                selite();

    TositeVienti vienti;

    int verokoodi = alvkoodi();
    bool maahantuonninvero = false;
    if( verokoodi == AlvKoodi::MAAHANTUONTI_VERO) {
        maahantuonninvero = true;
        verokoodi = AlvKoodi::MAAHANTUONTI;
    }


    vienti.setTyyppi( tyyppi + TositeVienti::KIRJAUS );
    if( vientiId_)
        vienti.setId( vientiId_);

    vienti.setTili( tilinumero() );
    vienti.setPvm(pvm);
    vienti.setSelite( rivinSelite );
    vienti.setKohdennus( kohdennus() );
    vienti.setMerkkaukset( merkkaukset() );
    vienti.setEra( era() );

    if( jaksoalkaa().isValid())
        vienti.setJaksoalkaa( jaksoalkaa() );
    if( jaksoalkaa().isValid() && jaksopaattyy().isValid() && jaksoalkaa().daysTo(jaksopaattyy()) > 0)
    vienti.setJaksoloppuu( jaksopaattyy());

    vienti.setAlvKoodi( verokoodi);
    if( verokoodi != AlvKoodi::EIALV && verokoodi != AlvKoodi::RAKENNUSPALVELU_MYYNTI &&
        verokoodi != AlvKoodi::YHTEISOMYYNTI_TAVARAT && verokoodi != AlvKoodi::YHTEISOMYYNTI_PALVELUT &&
        verokoodi != AlvKoodi::ALV0)
        vienti.setAlvProsentti( alvprosentti());

    if( !kumppani.isEmpty() )
        vienti.set(TositeVienti::KUMPPANI, kumppani );

    if( poistoaika()) {
        Tili* tili = kp()->tilit()->tili(tilinumero());
        if( tili && tili->tyyppi().onko(TiliLaji::TASAERAPOISTO)) {
            vienti.setTasaerapoisto( poistoaika());
            vienti.setEra(-1);
        }
    }


    Euro kirjattava = ( verokoodi == AlvKoodi::MYYNNIT_NETTO  || verokoodi == AlvKoodi::OSTOT_NETTO ||
                             verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI || verokoodi== AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
                             verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi== AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
                             verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI ||
                             verokoodi == AlvKoodi::MAAHANTUONTI_PALVELUT
                        ) && naytaNetto() ? netto : maara;

    if( !plusOnKredit ^ (kirjattava < Euro::Zero)) {
        vienti.setDebet( kirjattava.abs() );
    } else {
        vienti.setKredit( kirjattava.abs() );
    }

    qDebug() << " MAARA " << maara.display() << " NETTO " << netto.display() << " KIRJATTAVA " << kirjattava.display();

    vientilista.append( vienti );

    // ALV-SAAMINEN
    if( verokoodi == AlvKoodi::OSTOT_NETTO || verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO ||
          ((verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI || verokoodi == AlvKoodi::MAAHANTUONTI_PALVELUT )
           &&  alvvahennys() ) ) {

        TositeVienti palautus;
        palautus.setTyyppi( tyyppi + TositeVienti::ALVKIRJAUS );

        palautus.setPvm(pvm);
        if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_OSTO) {
            palautus.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).numero() );
            palautus.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO );
            palautus.setEra(-1);
        } else {
            palautus.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero());
            palautus.setAlvKoodi( AlvKoodi::ALVVAHENNYS + verokoodi );
        }
        if(plusOnKredit == vero < Euro::Zero )
            palautus.setDebet( vero.abs() );
        else
            palautus.setKredit( vero.abs());

        if( vahennysVientiId_ )
            palautus.setId( vahennysVientiId_ );

        palautus.setAlvProsentti( alvprosentti() );
        palautus.setSelite( rivinSelite );
        if( !kumppani.isEmpty() )
            palautus.set(TositeVienti::KUMPPANI, kumppani );
        vientilista.append(palautus);
    }

    // Alv-veron kirjaaminen
    if( verokoodi == AlvKoodi::MYYNNIT_NETTO || verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI ||
            verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI ||
            verokoodi == AlvKoodi::MAAHANTUONTI_PALVELUT )
    {

        const bool kaanteinen = verokoodi != AlvKoodi::MYYNNIT_NETTO && verokoodi != AlvKoodi::MAKSUPERUSTEINEN_MYYNTI;

        TositeVienti verorivi;
        verorivi.setTyyppi( tyyppi + TositeVienti::ALVKIRJAUS );
        verorivi.setPvm(pvm);
        if( verokoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
            verorivi.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::KOHDENTAMATONALVVELKA ).numero() );
            verorivi.setAlvKoodi( AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI);
            verorivi.setEra(-1);
        } else {
            verorivi.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero());
            verorivi.setAlvKoodi( AlvKoodi::ALVKIRJAUS + verokoodi);
        }

        if(!plusOnKredit ^ vero > Euro::Zero ^ kaanteinen)
            verorivi.setKredit( vero.abs() );
        else
            verorivi.setDebet( vero.abs());

        verorivi.setAlvProsentti( alvprosentti());
        verorivi.setSelite(rivinSelite);

        if( veroVientiId_ )
            verorivi.setId(veroVientiId_);

        if( !kumppani.isEmpty() )
            verorivi.set(TositeVienti::KUMPPANI, kumppani );
        vientilista.append(verorivi);
    }

    // Mahdollinen maahantuonnin veron kirjaamisen vastakirjaaminen
    if( maahantuonninvero ) {
        TositeVienti tuonti;
        tuonti.setTyyppi(tyyppi + TositeVienti::MAAHANTUONTIVASTAKIRJAUS);
        tuonti.setPvm(pvm);
        tuonti.setTili( tilinumero() );
        if( !plusOnKredit ^ ( netto < Euro::Zero))
            tuonti.setKredit(netto.abs());
        else
            tuonti.setDebet( netto.abs());

        tuonti.setSelite(rivinSelite);
        tuonti.setAlvKoodi(AlvKoodi::MAAHANTUONTI_VERO);
        if( !kumppani.isEmpty() )
            tuonti.set(TositeVienti::KUMPPANI, kumppani );
        if( maahantuontiVastaId_)
            tuonti.setId( maahantuontiVastaId_ );
        vientilista.append(tuonti);
    }
    // Jos ei oikeuta alv-vähennykseen, kirjataan myös tämä osuus menoksi
    if( (verokoodi == AlvKoodi::RAKENNUSPALVELU_OSTO || verokoodi == AlvKoodi::YHTEISOHANKINNAT_TAVARAT ||
            verokoodi == AlvKoodi::YHTEISOHANKINNAT_PALVELUT || verokoodi == AlvKoodi::MAAHANTUONTI ||
            verokoodi == AlvKoodi::MAAHANTUONTI_PALVELUT )
           &&  !alvvahennys()  ) {

        TositeVienti palautus;
        palautus.setPvm( pvm );
        palautus.setTyyppi( tyyppi + TositeVienti::VAHENNYSKELVOTON );
        palautus.setTili( tilinumero() );
        palautus.setAlvKoodi( AlvKoodi::VAHENNYSKELVOTON);
        palautus.setAlvProsentti( alvprosentti() );

        if( !plusOnKredit ^ ( vero < Euro::Zero))
            palautus.setDebet( vero.abs() );
        else
            palautus.setKredit( vero.abs());

        palautus.setSelite( rivinSelite );
        if( !kumppani.isEmpty() )
            palautus.set(TositeVienti::KUMPPANI, kumppani );
        if( vahentamatonVientiId_)
            palautus.setId( vahentamatonVientiId_ );
        vientilista.append(palautus);
    }

    return vientilista;
}

void ApuriRivi::setMaahantuonninAlv(int vientiId)
{
    setAlvkoodi( AlvKoodi::MAAHANTUONTI_VERO);
    maahantuontiVastaId_ = vientiId;
}

void ApuriRivi::setVahentamaton(int vientiId)
{
    alvvahennys_ = false;
    vahentamatonVientiId_ = vientiId;
}

void ApuriRivi::vaihdaEtumerkki()
{
    brutto_ = Euro::Zero - brutto_;
    netto_ = Euro::Zero - netto_;
}


