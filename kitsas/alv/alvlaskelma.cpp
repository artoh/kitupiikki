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
#include "alvlaskelma.h"
#include "db/kirjanpito.h"
#include "db/verotyyppimodel.h"
#include "model/tosite.h"
#include "model/tositeviennit.h"
#include "db/tositetyyppimodel.h"
#include "model/tositeliitteet.h"
#include "alvsivu.h"

#include <QDebug>

AlvLaskelma::AlvLaskelma(QObject *parent, const QString kielikoodi) :
  Raportteri (parent, kielikoodi),
  tosite_( new Tosite(this))
{

}

AlvLaskelma::~AlvLaskelma()
{
}

void AlvLaskelma::kirjoitaLaskelma()
{
    kirjoitaOtsikot();
    kirjoitaYhteenveto();
    kirjoitaErittely();
}

int AlvLaskelma::huojennusKuukaudet(const QDate &alku, const QDate &loppu)
{
    if( alku >= loppu)
        return 0;

    int kuukaudet = 0;
    QDate pvm = alku;

    // Ensimmäinen kuukausi
    if( alku.day() == 1)
        kuukaudet = 1;
    // Viimeinen kuukausi
    if( loppu.addDays(1).day() == 1 &&
        ( alku.month() != loppu.month() || alku.year() != loppu.year()) )
        kuukaudet++;

    pvm = pvm.addMonths(1);

    while( pvm < loppu && (pvm.year() != loppu.year() || pvm.month() != loppu.month())) {
        kuukaudet++;
        pvm = pvm.addMonths(1);
    }

    return kuukaudet;

}

void AlvLaskelma::kirjoitaOtsikot()
{
    rk.asetaOtsikko( kaanna("ARVONLISÄVEROLASKELMA"));
    rk.asetaKausiteksti( QString("%1 - %2").arg(alkupvm_.toString("dd.MM.yyyy"), loppupvm_.toString("dd.MM.yyyy") ) );

    rk.lisaaPvmSarake();
    rk.lisaaSarake("TOSITE12345");
    rk.lisaaVenyvaSarake();
    rk.lisaaVenyvaSarake();
    rk.lisaaSarake("24,00 ");
    rk.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa(kaanna("Pvm"));
    otsikko.lisaa(kaanna("Tosite"));
    otsikko.lisaa(kaanna("Asiakas/Toimittaja"));
    otsikko.lisaa(kaanna("Selite"));
    otsikko.lisaa("%",1,true);
    otsikko.lisaa("€",1,true);
    rk.lisaaOtsake(otsikko);
}

void AlvLaskelma::kirjoitaYhteenveto()
{

    RaporttiRivi otsikko;
    otsikko.lisaa(kaanna("Arvonlisäveroilmoituksen tiedot"),5);
    otsikko.lihavoi();
    otsikko.asetaKoko(14);

    rk.lisaaRivi(otsikko);
    rk.lisaaTyhjaRivi();

    if( kp()->onkoMaksuperusteinenAlv(loppupvm_)) {
        RaporttiRivi rivi;
        rivi.lisaa(kaanna("Maksuperusteinen arvonlisävero"),5);
        rk.lisaaRivi(rivi);
        rk.lisaaTyhjaRivi();
    }

    if( suhteutuskuukaudet_ ) {
        Euro huojennusraja = huojennusalku_ < QDate(2021,1,1) ? Euro("10000") : Euro("15000") ;
        if( liikevaihto_ < huojennusraja)
            huojennus_ = verohuojennukseen_;
        else {
            huojennus_ = verohuojennukseen_ - ( (liikevaihto_ - huojennusraja) * verohuojennukseen_ ) / ( Euro("30000") - huojennusraja );
        }
        if( huojennus_ > verohuojennukseen_)
            huojennus_ = verohuojennukseen_;
        if( huojennus_ < Euro(0) )
            huojennus_ = 0;
    }

    // Kotimaan myynti
    yvRivi(301, kaanna("Suoritettava %1%:n vero kotimaan myynnistä").arg(24), kotimaanmyyntivero(2400) );
    yvRivi(302, kaanna("Suoritettava %1%:n vero kotimaan myynnistä").arg(14), kotimaanmyyntivero(1400));
    yvRivi(303, kaanna("Suoritettava %1%:n vero kotimaan myynnistä").arg(10), kotimaanmyyntivero(1000));

    rk.lisaaTyhjaRivi();
    yvRivi(305, kaanna("Vero tavaraostoista muista EU-maista"),taulu_.summa(AlvKoodi::YHTEISOHANKINNAT_TAVARAT + AlvKoodi::ALVKIRJAUS));
    yvRivi(306,kaanna("Vero palveluostoista muista EU-maista"),taulu_.summa(AlvKoodi::YHTEISOHANKINNAT_PALVELUT + AlvKoodi::ALVKIRJAUS));
    yvRivi(304, kaanna("Vero tavaroiden maahantuonnista EU:n ulkopuolelta"), taulu_.summa( AlvKoodi::MAAHANTUONTI + AlvKoodi::ALVKIRJAUS ));
    yvRivi(318, kaanna("Vero rakentamispalveluiden ja metalliromun ostoista"), taulu_.summa(AlvKoodi::RAKENNUSPALVELU_OSTO + AlvKoodi::ALVKIRJAUS));

    rk.lisaaTyhjaRivi();

    yvRivi(307, kaanna("Verokauden vähennettävä vero"), taulu_.summa(200,299));

    rk.lisaaTyhjaRivi();

    maksettava_ = taulu_.summa(100,199) - taulu_.summa(200,299) - huojennus();
    yvRivi(308, kaanna("Maksettava vero / Palautukseen oikeuttava vero"), maksettava_);

    rk.lisaaTyhjaRivi();

    yvRivi(309, kaanna("0-verokannan alainen liikevaihto"), taulu_.summa(AlvKoodi::ALV0));

    rk.lisaaTyhjaRivi();
    yvRivi(311, kaanna("Tavaroiden myynnit muihin EU-maihin"), taulu_.summa(AlvKoodi::YHTEISOMYYNTI_TAVARAT));
    yvRivi(312, kaanna("Palveluiden myynnit muihin EU-maihin"), taulu_.summa(AlvKoodi::YHTEISOMYYNTI_PALVELUT));
    yvRivi(313, kaanna("Tavaraostot muista EU-maista"), taulu_.summa(AlvKoodi::YHTEISOHANKINNAT_TAVARAT));
    yvRivi(314, kaanna("Palveluostot muista EU-maista"), taulu_.summa(AlvKoodi::YHTEISOHANKINNAT_PALVELUT));

    rk.lisaaTyhjaRivi();

    yvRivi(310, kaanna("Tavaroiden maahantuonnit EU:n ulkopuolelta"), taulu_.summa(AlvKoodi::MAAHANTUONTI));

    rk.lisaaTyhjaRivi();

    yvRivi(319, kaanna("Rakentamispalveluiden ja metalliromun myynnit"), taulu_.summa(AlvKoodi::RAKENNUSPALVELU_MYYNTI));
    yvRivi(320, kaanna("Rakentamispalveluiden ja metalliromun ostot"), taulu_.summa(AlvKoodi::RAKENNUSPALVELU_OSTO));

    rk.lisaaTyhjaRivi();

    yvRivi(315, kaanna("Alarajahuojennukseen oikeuttava liikevaihto"), liikevaihto_ );
    yvRivi(316, kaanna("Alarajahuojennukseen oikeuttava vero"), verohuojennukseen_);
    yvRivi(317, kaanna("Alarajahuojennuksen määrä"), huojennus());
    if( liikevaihto_ && !huojennus()) {
        RaporttiRivi rivi;
        rivi.lisaa("", 2);
        rivi.lisaa(kaanna("Ei oikeutta alarajahuojennukseen"),3);
        rk.lisaaRivi(rivi);
    }

    rk.lisaaTyhjaRivi();

}

void AlvLaskelma::kirjaaVerot()
{
    Euro vero = taulu_.summa(100,199);
    Euro vahennys = taulu_.summa(200,299);

    QString selite = kaanna("Arvonlisävero %1 - %2")
            .arg( alkupvm_.toString("dd.MM.yyyy"), loppupvm_.toString("dd.MM.yyyy"));

    if( vero ) {
        TositeVienti verot;
        verot.setSelite(selite);
        verot.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero() );
        if(vero > Euro(0))
            verot.setDebet( vero );
        else
            verot.setKredit( Euro(0) - vero);
        verot.setAlvKoodi( AlvKoodi::TILITYS );

        if(vero)
            lisaaKirjausVienti(verot);
    }
    if( vahennys )
    {
        TositeVienti vahennysRivi;
        vahennysRivi.setSelite(selite);
        vahennysRivi.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero() );
        if( vahennys > Euro::Zero)
            vahennysRivi.setKredit( vahennys );
        else
            vahennysRivi.setDebet( Euro::Zero - vahennys );
        vahennysRivi.setAlvKoodi( AlvKoodi::TILITYS);

        if(vahennys)
            lisaaKirjausVienti( vahennysRivi );
    }
    if( vero != vahennys) {
        TositeVienti maksu;
        maksu.setSelite( selite );
        if( vero > vahennys && kp()->asetukset()->onko("AlvMaksutilinKautta") &&
                kp()->asetukset()->luku("AlvMaksettava")) {
            maksu.setTili( kp()->asetukset()->luku("AlvMaksettava") );
        } else if( vahennys > vero && kp()->asetukset()->onko("AlvMaksutilinKautta") &&
                   kp()->asetukset()->luku("AlvPalautettava")) {
            maksu.setTili( kp()->asetukset()->luku("AlvPalautettava"));
        } else if( vahennys > vero && kp()->asetukset()->onko("AlvPalautusSaatavaTilille")) {
            maksu.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).numero() );
        } else {
            maksu.setTili( kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).numero() );
        }

        maksu.setAlvKoodi( AlvKoodi::TILITYS );
        if( vero > vahennys )
            maksu.setKredit(  vero - vahennys );
        else
            maksu.setDebet(  vahennys - vero );

        lisaaKirjausVienti( maksu );
    }
}

void AlvLaskelma::kirjoitaErittely()
{
    RaporttiRivi otsikko;
    otsikko.lisaa(kaanna("Erittely"),5);
    otsikko.lihavoi();
    otsikko.asetaKoko(14);

    rk.lisaaRivi(otsikko);
    rk.lisaaTyhjaRivi();


    QMapIterator<int, KoodiTaulu> koodiIter(taulu_.koodit);
    while( koodiIter.hasNext()) {
        koodiIter.next();
        int koodi = koodiIter.key();

        const KoodiTaulu &taulu = koodiIter.value();
        QMapIterator<int, KantaTaulu> kantaIter(taulu.kannat);
        while( kantaIter.hasNext()) {
            kantaIter.next();
            double verokanta = kantaIter.key() / 100.0;

            RaporttiRivi kantaOtsikko;
            kantaOtsikko.lisaa( kp()->alvTyypit()->yhdistelmaSeliteKoodilla(koodi, kielikoodi_), 4 );
            kantaOtsikko.lisaa( QString("%L1").arg(verokanta,0,'f',0));
            kantaOtsikko.lisaa( kantaIter.value().summa(debetistaKoodilla(koodi)) );
            kantaOtsikko.lihavoi();
            rk.lisaaRivi(kantaOtsikko);

            QMapIterator<int,TiliTaulu> tiliIter( kantaIter.value().tilit );
            while( tiliIter.hasNext()) {
                tiliIter.next();

                RaporttiRivi tiliOtsikko;
                tiliOtsikko.lisaa( kp()->tilit()->tiliNumerolla( tiliIter.key() ).nimiNumero(), 4 );
                rk.lisaaRivi(tiliOtsikko);

                for(auto& vienti : tiliIter.value().viennit) {
                    RaporttiRivi rivi;
                    rivi.lisaa( vienti.value("pvm").toDate() );                    
                    QVariantMap tositeMap = vienti.value("tosite").toMap();
                    rivi.lisaa( kp()->tositeTunnus(tositeMap.value("tunniste").toInt(),
                                tositeMap.value("pvm").toDate(),
                                tositeMap.value("sarja").toString(),
                                true));

                    QString selite = vienti.value("selite").toString();
                    QString kumppani = vienti.value("kumppani").toMap().value("nimi").toString();

                    rivi.lisaa(kumppani);
                    rivi.lisaa(selite == kumppani ? "" : selite);

                    rivi.lisaa(  QString("%L1").arg(verokanta,0,'f',0) );


                    Euro debet = Euro::fromVariant(vienti.value("debet"));
                    Euro kredit = Euro::fromVariant(vienti.value("kredit"));

                    if( debetistaKoodilla( koodi ) )
                        rivi.lisaa( debet - kredit );
                    else
                        rivi.lisaa( kredit - debet );

                    rk.lisaaRivi( rivi );
                }
                // Tilin summa
                RaporttiRivi tiliSumma;
                tiliSumma.lisaa(QString(), 4);
                tiliSumma.lisaa(  QString("%L1").arg(verokanta,0,'f',0) );
                tiliSumma.lisaa( tiliIter.value().summa( debetistaKoodilla(koodi) ) );
                tiliSumma.viivaYlle();
                rk.lisaaRivi(tiliSumma);
                rk.lisaaTyhjaRivi();
            }
        }
    }
    // Marginaalierittely
    for( RaporttiRivi& rivi : marginaaliRivit_)
        rk.lisaaRivi(rivi);
}

void AlvLaskelma::yvRivi(int koodi, const QString &selite, Euro euro)
{
    if( euro ) {
        RaporttiRivi rivi;
        rivi.lisaa(QString(), 1);
        rivi.lisaa( QString::number(koodi));
        rivi.lisaa(selite, 3);
        rivi.lisaa( euro);
        if( koodi == 308)   // #1050: Lihavoidaan varsinainen maksurivi
            rivi.lihavoi();
        rk.lisaaRivi(rivi);
    }
    if( koodi && euro)
        koodattu_.insert(koodi, euro.cents() );
}

Euro AlvLaskelma::kotimaanmyyntivero(int prosentinsadasosa)
{
    return taulu_.koodit.value(AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa() +
            taulu_.koodit.value(AlvKoodi::MYYNNIT_BRUTTO + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa() +
            taulu_.koodit.value(AlvKoodi::MAKSUPERUSTEINEN_MYYNTI + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa() +
            taulu_.koodit.value(AlvKoodi::MYYNNIT_MARGINAALI + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa() +
            taulu_.koodit.value(AlvKoodi::ENNAKKOLASKU_MYYNTI + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa() +
            taulu_.koodit.value(AlvKoodi::MAAHANTUONTI_PALVELUT + AlvKoodi::ALVKIRJAUS).kannat.value(prosentinsadasosa).summa();
}



void AlvLaskelma::tilaaMaksuperusteisenTosite()
{
    if( !nollattavatErat_.isEmpty()) {
        // Ajan perusteella nollattavaa
        QPair<int,qlonglong> nollattava = nollattavatErat_.takeFirst();
        KpKysely *kysely = kpk("/tositteet");
        kysely->lisaaAttribuutti("vienti", nollattava.first);
        connect( kysely, &KpKysely::vastaus, this, [this, nollattava] (QVariant* var) { this->maksuperusteTositesaapuu(var, nollattava.second);});
        kysely->kysy();
        nollatutErat_.insert(nollattava.first);
    } else {
        int tosite = 0;
        int era = 0;
        qlonglong saldo = 0;

        while (!maksuperusteTositteet_.isEmpty()) {
            int tosite = maksuperusteTositteet_.takeLast();
            int era = maksuperusteiset_.value(tosite).first;
            Euro saldo = maksuperusteiset_.value(tosite).second;
            if( !nollatutErat_.contains(era) && saldo )
                break;
        }
        if( !tosite)
            viimeistele();
        else {

            KpKysely *kysely = kpk("/tositteet");
            kysely->lisaaAttribuutti("vienti",era);

            connect( kysely, &KpKysely::vastaus, this, [this, saldo] (QVariant* var) {this->maksuperusteTositesaapuu(var, saldo);});
            kysely->kysy();
        }
    }
}

void AlvLaskelma::kasitteleMaksuperusteinen(const QVariantMap &map)
{
    // Myyntisaatavat ja ostosaatavat, jotka eivät ole
    // aloita uutta tase-erää, saattavat olla maksuja

    Tili* tili = kp()->tilit()->tili(map.value("tili").toInt());
    if( tili ) {
        int tositeId = map.value("tosite").toMap().value("id").toInt();
        qlonglong debet = qRound64( map.value("debet").toDouble() * 100.0 );
        qlonglong kredit = qRound64( map.value("kredit").toDouble() * 100.0 );
        int era = map.value("era").toMap().value("id").toInt();

        if( tili->onko(TiliLaji::MYYNTISAATAVA) || tili->onko(TiliLaji::OSTOVELKA)) {
            maksuperusteiset_.insert( tositeId, qMakePair(era, debet - kredit));
        } else if( tili->onko(TiliLaji::RAHAVARAT)) {
            if( !maksuperusteTositteet_.contains(tositeId))
                maksuperusteTositteet_.append(tositeId);
        }
    }
}

void AlvLaskelma::maksuperusteTositesaapuu(QVariant *variant, Euro euro)
{
    QVariantMap map = variant->toMap();
    QVariantList viennit = map.value("viennit").toList();
    TositeVienti vasta = viennit.value(0).toMap();
    Euro vastaeuro = vasta.kreditEuro() - vasta.debetEuro();

    for(QVariant& item : viennit) {
        TositeVienti vienti = item.toMap();
        if( qAbs(vienti.era().value("saldo").toDouble()) < 1e-5) {
            // Jos vero on jo maksettu, ei makseta uudelleen...
        } else if( vienti.alvKoodi() == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI) {
            TositeVienti kohdentamattomasta;
            kohdentamattomasta.setTili(vienti.tili());            
            Euro eurot = vienti.kreditEuro() * euro / vastaeuro;
            kohdentamattomasta.setDebet( eurot );
            kohdentamattomasta.setEra(vienti.era());
            kohdentamattomasta.setAlvProsentti(vienti.alvProsentti());
            QString selite = kaanna("Maksuperusteinen alv %1 %2").arg(vienti.pvm().toString("dd.MM.yyyy"), vienti.selite());
            kohdentamattomasta.setSelite(selite);
            kohdentamattomasta.setAlvKoodi(AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_MYYNTI);
            lisaaKirjausVienti(kohdentamattomasta);

            TositeVienti kohdennettuun;
            kohdennettuun.setTili(kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero());
            kohdennettuun.setKredit(eurot);
            kohdennettuun.setSelite(selite);
            kohdennettuun.setAlvProsentti(vienti.alvProsentti());
            kohdennettuun.setAlvKoodi(AlvKoodi::MAKSUPERUSTEINEN_MYYNTI + AlvKoodi::ALVKIRJAUS);
            lisaaKirjausVienti(kohdennettuun);
        } else if( vienti.alvKoodi() == AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO) {
            TositeVienti kohdentamattomasta;
            kohdentamattomasta.setTili(vienti.tili());
            Euro eurot = vienti.debetEuro() * euro / vastaeuro;
            kohdentamattomasta.setKredit( eurot );
            kohdentamattomasta.setEra(vienti.era());
            kohdentamattomasta.setAlvProsentti(vienti.alvProsentti());
            QString selite = kaanna("Maksuperusteinen alv %1 %2").arg(vienti.pvm().toString("dd.MM.yyyy"), vienti.selite());
            kohdentamattomasta.setSelite(selite);
            kohdentamattomasta.setAlvKoodi(AlvKoodi::MAKSUPERUSTEINEN_KOHDENTAMATON + AlvKoodi::MAKSUPERUSTEINEN_OSTO);
            lisaaKirjausVienti(kohdentamattomasta);

            TositeVienti kohdennettuun;
            kohdennettuun.setTili(kp()->tilit()->tiliTyypilla(TiliLaji::ALVSAATAVA).numero());
            kohdennettuun.setDebet(eurot);
            kohdennettuun.setSelite(selite);
            kohdennettuun.setAlvProsentti(vienti.alvProsentti());
            kohdennettuun.setAlvKoodi(AlvKoodi::MAKSUPERUSTEINEN_OSTO + AlvKoodi::ALVVAHENNYS);
            lisaaKirjausVienti(kohdennettuun);
        }

    }
    tilaaMaksuperusteisenTosite();
}

void AlvLaskelma::tilaaNollausLista(const QDate &pvm)
{
    KpKysely *kysely = kpk("/erat");
    kysely->lisaaAttribuutti("tili", kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVVELKA).numero());
    connect( kysely, &KpKysely::vastaus, this, [this,pvm] (QVariant *data) { this->nollaaMaksuperusteisetErat(data, pvm);} );
    kysely->kysy();

    kysely = kpk("/erat");
    kysely->lisaaAttribuutti("tili", kp()->tilit()->tiliTyypilla(TiliLaji::KOHDENTAMATONALVSAATAVA).numero());
    connect( kysely, &KpKysely::vastaus, this, [this,pvm] (QVariant *data) { this->nollaaMaksuperusteisetErat(data, pvm);} );
    kysely->kysy();
}

void AlvLaskelma::nollaaMaksuperusteisetErat(QVariant *variant, const QDate& pvm)
{
    QVariantList list = variant->toList();
    for( auto& item : list) {
        QVariantMap map = item.toMap();
        if( map.value("pvm").toDate() > pvm)
            continue;
        int eraid = map.value("id").toInt();
        qlonglong saldo = qRound64(map.value("avoin").toDouble() * 100.0);
        nollattavatErat_.append(qMakePair(eraid, saldo));
    }
    nollattavatHaut_--;
    if( !nollattavatHaut_)
        hae();
}

void AlvLaskelma::laskeMarginaaliVerotus(int kanta)
{
    KoodiTaulu taulu = taulu_.koodit.value(AlvKoodi::MYYNNIT_MARGINAALI);
    KantaTaulu ktaulu = taulu.kannat.value(kanta);
    Euro myynti = ktaulu.summa();

    KoodiTaulu ostotaulu = taulu_.koodit.value(AlvKoodi::OSTOT_MARGINAALI);
    KantaTaulu oktaulu = ostotaulu.kannat.value(kanta);
    Euro ostot = oktaulu.summa(true);
    Euro alijaama = kp()->alvIlmoitukset()->marginaalialijaama(alkupvm_.addDays(-1), kanta);

    Euro marginaali = myynti - ostot - alijaama;      // TODO: Alijäämän lisäys
    Euro vero = Euro::fromDouble( kanta / (10000.0 + kanta) *  marginaali.toDouble() );

    if( myynti || ostot ) {
        marginaaliRivit_.append(RaporttiRivi());
        marginaaliRivi(kaanna("Voittomarginaalimenettely myynti"),kanta,myynti);
        marginaaliRivi(kaanna("Voittomarginaalimenettely ostot"), kanta, ostot);
        if( alijaama )
            marginaaliRivi(kaanna("Aiempi alijäämä"), kanta, alijaama);
        if( marginaali > Euro::Zero || kp()->asetukset()->onko("AlvMatkatoimisto")) {
            marginaaliRivi(kaanna("Marginaali"), kanta, marginaali);
            marginaaliRivi(kaanna("Vero"),kanta,vero);
        } else if( marginaali < Euro::Zero) {
            marginaaliRivi(kaanna("Alijäämä"), kanta, Euro::Zero -marginaali);
        }
    }

    if( vero > Euro::Zero || (vero < Euro::Zero && kp()->asetukset()->onko("AlvMatkatoimisto"))) {
        // Marginaalivero kirjataan kaikille marginaalitileille
        QMapIterator<int,TiliTaulu> kirjausIter(ktaulu.tilit);
        while( kirjausIter.hasNext()) {
            kirjausIter.next();
            int tili = kirjausIter.key();
            Euro tilinmyynti = kirjausIter.value().summa();

            QString selite = kaanna("Voittomarginaalivero (Verokanta %1 %, osuus %2 %)")
                    .arg(kanta / 100.0, 0, 'f', 2)
                    .arg(tilinmyynti.cents() * 100.0 / myynti.cents(), 0, 'f', 2);
            TositeVienti tililta;
            tililta.setTili(tili);
            Euro eurot = Euro::fromDouble(tilinmyynti.toDouble() / myynti.toDouble() * vero.toDouble());
            if( eurot > Euro::Zero)
                tililta.setDebet( eurot );
            else
                tililta.setKredit( Euro::Zero-eurot);

            tililta.setAlvProsentti(kanta / 100.0);
            tililta.setSelite(selite);
            tililta.setAlvKoodi(AlvKoodi::MYYNNIT_MARGINAALI + AlvKoodi::TILITYS);
            tililta.setTyyppi(TositeVienti::BRUTTOOIKAISU);
            lisaaKirjausVienti(tililta);

            TositeVienti tilille;
            tilille.setTili(kp()->tilit()->tiliTyypilla(TiliLaji::ALVVELKA).numero());
            if( eurot > Euro::Zero)
                tilille.setKredit( eurot );
            else
                tilille.setDebet( Euro::Zero-eurot);
            tilille.setAlvProsentti( kanta / 100.0);
            tilille.setSelite( selite );
            tilille.setAlvKoodi(AlvKoodi::MYYNNIT_MARGINAALI + AlvKoodi::ALVKIRJAUS);
            lisaaKirjausVienti(tilille);
        }

    } else if( marginaali < Euro::Zero) {
        // Marginaaliveron alijäämä laitetaan muistiin
        marginaaliAlijaamat_.insert( QString::number(kanta / 100,'f',2), Euro::Zero - marginaali);
    }

}

void AlvLaskelma::marginaaliRivi(const QString selite, int kanta, Euro summa)
{
    RaporttiRivi rr;
    rr.lisaa("",2);
    rr.lisaa(selite);
    rr.lisaa(QString("%L1").arg(kanta / 100.0,0,'f',0) );
    rr.lisaa(summa);
    marginaaliRivit_.append(rr);

}

void AlvLaskelma::hae()
{
    // Jos maksuperusteinen nollaus, tehdään se ensin
    // Jos maksuperusteinen käytössä, ovat erääntyvät vuosi sitten tehdyt erät
    if( kp()->onkoMaksuperusteinenAlv(loppupvm_) && nollattavatHaut_)
        tilaaNollausLista(alkupvm_.addYears(-1));
    else if( kp()->asetukset()->pvm("MaksuAlvLoppuu") == alkupvm_ && nollattavatHaut_)
        // Kun maksuperusteinen alv päätetään, erääntyvät kaikki erät
        tilaaNollausLista( alkupvm_);
    else {

        KpKysely* kysely = kpk("/viennit");
        kysely->lisaaAttribuutti("alkupvm", alkupvm_);
        kysely->lisaaAttribuutti("loppupvm", loppupvm_);
        connect( kysely, &KpKysely::vastaus, this, &AlvLaskelma::viennitSaapuu);
        kysely->kysy();
    }
}

void AlvLaskelma::laske(const QDate &alkupvm, const QDate &loppupvm)
{
    alkupvm_ = alkupvm;
    loppupvm_ = loppupvm;
    hae();
}

void AlvLaskelma::tallennaViennit(const QVariantList &viennit, bool maksuperusteinen)
{
    taulu_.koodit.clear();
    QVariantList lista = viennit;
    for(auto& item : lista) {
        QVariantMap map = item.toMap();
        if( map.value("alvkoodi").toInt() && map.value("tosite").toMap().value("tyyppi").toInt() != TositeTyyppi::ALVLASKELMA )
            taulu_.lisaa(map);
        if( maksuperusteinen )
            kasitteleMaksuperusteinen(map);
    }
}


void AlvLaskelma::viennitSaapuu(QVariant *viennit)
{
    bool maksuperusteinen = kp()->onkoMaksuperusteinenAlv(loppupvm_);
    tallennaViennit( viennit->toList(), maksuperusteinen);
    // Jos maksuperuste, käsitellään ko. tositteet
    tilaaMaksuperusteisenTosite();
}

void AlvLaskelma::haeHuojennusJosTarpeen()
{
    QDate huojennusloppu;


    if( loppupvm_ == kp()->tilikaudet()->tilikausiPaivalle(loppupvm_).paattyy() && alkupvm_.daysTo(loppupvm_) < 32 ) {
        huojennusalku_ = kp()->tilikaudet()->tilikausiPaivalle(loppupvm_).alkaa();
        huojennusloppu = loppupvm_;
    } else if( loppupvm_.month() == 12 && loppupvm_.day() == 31 && alkupvm_.daysTo(loppupvm_) > 31) {
        // Jos alv-kausi muu kuin kuukausi, lasketaan verovuoden mukaisesti
        huojennusloppu = loppupvm_;
        huojennusalku_ = QDate(loppupvm_.year(),1,1);
        QDate alvalkaa = kp()->asetukset()->pvm("AlvAlkaa");
        if( alvalkaa.isValid() && alvalkaa > huojennusalku_)
            huojennusalku_ = alvalkaa;
    }


    if( huojennusalku_.isValid()) {
        suhteutuskuukaudet_ = huojennusKuukaudet(huojennusalku_, loppupvm_);

        // Sitten tehdään huojennushaku
        KpKysely* kysely = kpk("/viennit");
        kysely->lisaaAttribuutti("alkupvm", huojennusalku_);
        kysely->lisaaAttribuutti("loppupvm", huojennusloppu);
        connect( kysely, &KpKysely::vastaus, this, &AlvLaskelma::laskeHuojennus);
        kysely->kysy();

    } else {
        viimeViimeistely();
    }
}

void AlvLaskelma::laskeHuojennus(QVariant *viennit)
{ 

    liikevaihto_ = 0;  

    Euro veroon;
    Euro vahennys;

    QList<TositeVienti> lista;

    for(const auto& var : viennit->toList()) {
        TositeVienti vienti = var.toMap();
        if( vienti.value("tosite").toMap().value("tyyppi").toInt() != TositeTyyppi::ALVLASKELMA || vienti.value("pvm").toDate() <= alkupvm_)
            lista.append(vienti);
    }

    lista.append(tosite_->viennit()->viennit());

    qDebug() << " =======HUOJENNUSLASKELMA==================";

    for( const auto& vienti : lista ) {

        if( vienti.tyyppi() == TositeVienti::BRUTTOOIKAISU)
            continue;

        int alvkoodi = vienti.alvKoodi();
        Euro debetEuro = vienti.debetEuro();
        Euro kreditEuro = vienti.kreditEuro();


        qDebug() << vienti.pvm().toString("dd.MM.yyyy ") << " D " << vienti.debet() << " K "  << vienti.kredit() << " ALV " << vienti.alvKoodi() << " " << vienti.selite();

        if( alvkoodi > 0 && alvkoodi < 100) {
            // Tämä on veron tai vähennyksen peruste
            if( alvkoodi == AlvKoodi::MYYNNIT_NETTO ||
                    alvkoodi == AlvKoodi::YHTEISOMYYNTI_TAVARAT ||
                    alvkoodi == AlvKoodi::ALV0 ||
                    alvkoodi == AlvKoodi::RAKENNUSPALVELU_MYYNTI ) {
                liikevaihto_ += kreditEuro - debetEuro;
                qDebug() << " A Liikevaihtoon " << kreditEuro-debetEuro;
            } else if( alvkoodi == AlvKoodi::MYYNNIT_BRUTTO) {
                Euro brutto = kreditEuro - debetEuro;
                Euro netto = Euro::fromDouble( ( 100 * brutto.toDouble() / (100 + vienti.alvProsentti()) )) ;
                liikevaihto_ += netto;
                veroon += brutto - netto;
                qDebug() << " B Liikevaihtoon " << netto << " Veroon " << brutto-netto;
            } else if( alvkoodi == AlvKoodi::OSTOT_BRUTTO) {
                Euro brutto = debetEuro - kreditEuro;
                Euro netto = Euro::fromDouble( ( 100 * brutto.toDouble() / (100 + vienti.alvProsentti()) )) ;
                vahennys += brutto - netto;
                qDebug() << " C Vähennys " << brutto-netto;
            } else if( alvkoodi == AlvKoodi::MYYNNIT_MARGINAALI) {
                liikevaihto_ += kreditEuro - debetEuro;
                qDebug() << " D Liikevaihtoon " << kreditEuro-debetEuro;
            }
        } else if( alvkoodi > 100 && alvkoodi < 200 && vienti.alvProsentti() > 1e-5) {
            // Tämä on maksettava vero
            if( alvkoodi == AlvKoodi::MYYNNIT_NETTO + AlvKoodi::ALVKIRJAUS ||
                alvkoodi == AlvKoodi::MYYNNIT_BRUTTO + AlvKoodi::ALVKIRJAUS ) {
                veroon += kreditEuro - debetEuro;
                qDebug() << " E Veroon " << kreditEuro-debetEuro;
            } else if( alvkoodi == AlvKoodi::MYYNNIT_MARGINAALI + AlvKoodi::ALVKIRJAUS) {
                qlonglong vero = kreditEuro - debetEuro;
                veroon += vero;
                liikevaihto_ -= vero;
                qDebug() << "F Liikevaihtoon " << 0 - vero << " Veroon " << vero;
            } else if( alvkoodi == AlvKoodi::MAKSUPERUSTEINEN_MYYNTI + AlvKoodi::ALVKIRJAUS ||
                       alvkoodi == AlvKoodi::ENNAKKOLASKU_MYYNTI + AlvKoodi::ALVKIRJAUS ) {
                // Käytettyjen tavaroiden sekä taide-, keräily- ja antiikkiesineiden marginaaliverojärjestelmää
                // ja matkatoimistopalvelujen marginaaliverojärjestelmää sovellettaessa liikevaihtoon
                // luetaan ostajalta veloitettu myyntihinta vähennettynä myynnistä suoritetun
                // arvonlisäveron osuudella.

                // Maksuperusteisessa alvissa veron peruste voi olla kirjattu toiselle verokaudelle, joten
                // se joudutaan laskemaan samoin verokirjauksesta

                Euro vero = kreditEuro - debetEuro;
                double veroprossa = vienti.alvProsentti();
                qlonglong liikevaihtoon = Euro::fromDouble( 100 * vero.toDouble() /  veroprossa);
                veroon += vero;
                liikevaihto_ += liikevaihtoon;
                qDebug() << " G Liikevaihtoon " << liikevaihtoon << " Veroon " << vero;
            }
        } else if( alvkoodi > 200 && alvkoodi < 300) {
            // Kaikki ostojen alv-vähennykset lasketaan huojennukseen
            vahennys += debetEuro - kreditEuro;
            qDebug() << " H Vähennykseen " << debetEuro-kreditEuro;
        }
    }

    qDebug() << " ================================= ";
    qDebug() << " Oikeuttava Liikevaihto " << liikevaihto_;

    qDebug() << " Huomioitavat verot " << veroon;
    qDebug() << " Huomioitavat vähennykset " << vahennys;
    verohuojennukseen_ = veroon - vahennys;

    qDebug() << " Oikeuttava vero " << verohuojennukseen_;

    liikevaihto_ = liikevaihto_ * 12 / (suhteutuskuukaudet_ ? suhteutuskuukaudet_ : 1);

    qDebug() << " Suhteutettu "<< liikevaihto_;

    viimeViimeistely();

}

void AlvLaskelma::tallennusValmis()
{
    kp()->alvIlmoitukset()->lataa();
    emit tallennettu();
}

void AlvLaskelma::viimeistele()
{

    oikaiseBruttoKirjaukset();
    laskeMarginaaliVerotus(2400);
    laskeMarginaaliVerotus(1400);
    laskeMarginaaliVerotus(1000);
    haeHuojennusJosTarpeen();
}

void AlvLaskelma::viimeViimeistely()
{
    kirjoitaLaskelma();
    kirjaaVerot();
    emit valmis( rk );
}

void AlvLaskelma::kirjaaHuojennus()
{
    if( !huojennus() || !kp()->asetukset()->luku("AlvHuojennusTili"))
        return;

    QString selite = kaanna("Arvonlisäveron alarajahuojennus");

    bool maksutilinkautta = ( kp()->asetukset()->onko("AlvMaksutilinKautta") && kp()->asetukset()->luku("AlvMaksettava") && kp()->asetukset()->luku("AlvPalautettava") );

    Euro kaudenvero = taulu_.summa(100,199) - taulu_.summa(200,299);
    if( kaudenvero > Euro::Zero &&
            ( kp()->asetukset()->onko("AlvPalautusSaatavatilille") || maksutilinkautta ) &&
            huojennus() > kaudenvero) {
        TositeVienti huojennusVerolta;
        huojennusVerolta.setTili( maksutilinkautta ?
                                      kp()->asetukset()->luku("AlvMaksettava") :
                                      kp()->tilit()->tiliTyypilla(TiliLaji::VEROVELKA).numero()
                                      );
        huojennusVerolta.setSelite( selite );
        huojennusVerolta.setDebet( kaudenvero );
        lisaaKirjausVienti( huojennusVerolta );

        TositeVienti huojennusSaatavaan;
        huojennusSaatavaan.setTili( maksutilinkautta ?
                                        kp()->asetukset()->luku("AlvPalautettava") :
                                        kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).numero()  );
        huojennusSaatavaan.setSelite( selite );
        huojennusSaatavaan.setDebet( huojennus() - kaudenvero );
        lisaaKirjausVienti( huojennusSaatavaan );

    } else {
        TositeVienti huojennettava;
        huojennettava.setTili( maksutilinkautta ? kp()->asetukset()->luku("AlvMaksettava") : kp()->tilit()->tiliTyypilla(TiliLaji::VEROSAATAVA).numero() );
        huojennettava.setSelite( selite );
        huojennettava.setDebet( huojennus() );
        lisaaKirjausVienti( huojennettava );
    }

    TositeVienti huojentaja;
    huojentaja.setTili( kp()->asetukset()->luku("AlvHuojennusTili") );
    huojentaja.setSelite( selite );
    huojentaja.setKredit( huojennus() );
    lisaaKirjausVienti( huojentaja );
}

void AlvLaskelma::tallenna()
{
    tosite_->setData( Tosite::PVM, loppupvm_ );
    tosite_->setData( Tosite::OTSIKKO, kaanna("Arvonlisäveroilmoitus %1 - %2")
                     .arg(alkupvm_.toString("dd.MM.yyyy"), loppupvm_.toString("dd.MM.yyyy")));
    tosite_->setData( Tosite::TYYPPI, TositeTyyppi::ALVLASKELMA  );
    tosite_->asetaSarja( kp()->tositeTyypit()->sarja( TositeTyyppi::ALVLASKELMA ) ) ;

    tosite_->liitteet()->lisaa( rk.pdf(), "alv.pdf", "alv" );

    QVariantMap lisat;
    QVariantMap koodit;
    QMapIterator<int,Euro> iter(koodattu_);
    while( iter.hasNext()) {
        iter.next();
        koodit.insert( QString::number( iter.key() ), iter.value().cents());
    }
    if( kp()->onkoMaksuperusteinenAlv(loppupvm_))
        koodit.insert("337",1);

    lisat.insert("koodit", koodit);
    lisat.insert("kausialkaa", alkupvm_);
    lisat.insert("kausipaattyy", loppupvm_);
    lisat.insert("erapvm", AlvIlmoitustenModel::erapaiva(loppupvm_));
    lisat.insert("maksettava", maksettava());
    if( !marginaaliAlijaamat_.isEmpty() )
        lisat.insert("marginaalialijaama", marginaaliAlijaamat_);
    tosite_->setData( Tosite::ALV, lisat);


    connect( tosite_, &Tosite::talletettu, this, &AlvLaskelma::tallennusValmis);
    tosite_->tallenna();
}

void AlvLaskelma::lisaaKirjausVienti(TositeVienti vienti)
{
    vienti.setPvm( loppupvm_ );
    taulu_.lisaa(vienti);

    tosite_->viennit()->lisaa(vienti);
}

void AlvLaskelma::oikaiseBruttoKirjaukset()
{
    QMapIterator<int,KantaTaulu> myyntiIter( taulu_.koodit.value( AlvKoodi::MYYNNIT_BRUTTO ).kannat );
    while( myyntiIter.hasNext())
    {
        myyntiIter.next();
        QMapIterator<int,TiliTaulu> tiliIter( myyntiIter.value().tilit );
        while( tiliIter.hasNext())
        {
            tiliIter.next();
            int tili = tiliIter.key();
            Euro brutto = tiliIter.value().summa();
            int sadasosaprosentti = myyntiIter.key();

            Euro netto = Euro::fromDouble( brutto.toDouble() * 10000.0 / ( 10000.0 + sadasosaprosentti) );
            Euro vero = brutto - netto;

            QString selite = kaanna("Bruttomyyntien oikaisu %3 BRUTTO %1, NETTO %2")
                    .arg(brutto.display(true), netto.display(true), kp()->tilit()->tiliNumerolla(tili).nimiNumero() );

            TositeVienti pois;
            pois.setTili(tili);
            if( vero > Euro::Zero)
                pois.setDebet( vero);
            else
                pois.setKredit(Euro::Zero - vero);
            pois.setAlvKoodi( AlvKoodi::MYYNNIT_BRUTTO );
            pois.setAlvProsentti( sadasosaprosentti / 100.0 );
            pois.setTyyppi(TositeVienti::BRUTTOOIKAISU);
            pois.setSelite(selite);
            lisaaKirjausVienti( pois );

            TositeVienti veroon;
            veroon.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::ALVVELKA ).numero() );
            if(vero > Euro::Zero)
                veroon.setKredit( vero);
            else
                veroon.setDebet(Euro::Zero - vero);
            veroon.setAlvKoodi( AlvKoodi::ALVKIRJAUS + AlvKoodi::MYYNNIT_BRUTTO);
            veroon.setTyyppi( TositeVienti::BRUTTOOIKAISU );
            veroon.setAlvProsentti( sadasosaprosentti / 100.0);
            veroon.setSelite(selite);
            lisaaKirjausVienti( veroon );
        }
    }

    QMapIterator<int,KantaTaulu> ostoIter( taulu_.koodit.value( AlvKoodi::OSTOT_BRUTTO ).kannat );
    while( ostoIter.hasNext())
    {
        ostoIter.next();
        QMapIterator<int,TiliTaulu> tiliIter( ostoIter.value().tilit );
        while( tiliIter.hasNext())
        {
            tiliIter.next();
            int tili = tiliIter.key();
            Euro brutto = tiliIter.value().summa(true);
            int sadasosaprosentti = ostoIter.key();

            Euro netto = Euro::fromDouble(brutto.toDouble() * 10000.0 / ( 10000 + sadasosaprosentti));
            Euro vero = brutto - netto;

            QString selite = kaanna("Brutto-ostojen oikaisu %3 BRUTTO %1, NETTO %2")
                    .arg(brutto.display(true), netto.display(true), kp()->tilit()->tiliNumerolla(tili).nimiNumero() );

            TositeVienti pois;
            pois.setTili(tili);
            if( vero > Euro::Zero)
                pois.setKredit( vero);
            else
                pois.setDebet( Euro::Zero - vero);

            pois.setAlvKoodi( AlvKoodi::OSTOT_BRUTTO );
            pois.setAlvProsentti( sadasosaprosentti / 100.0 );
            pois.setSelite(selite);
            pois.setTyyppi( TositeVienti::BRUTTOOIKAISU );
            lisaaKirjausVienti( pois );

            TositeVienti veroon;
            veroon.setTili( kp()->tilit()->tiliTyypilla( TiliLaji::ALVSAATAVA ).numero() );
            if( vero > Euro::Zero)
                veroon.setDebet( vero);
            else
                veroon.setKredit( Euro::Zero - vero );
            veroon.setAlvKoodi( AlvKoodi::ALVVAHENNYS + AlvKoodi::OSTOT_BRUTTO);
            veroon.setAlvProsentti( sadasosaprosentti / 100.0);
            veroon.setSelite(selite);
            veroon.setTyyppi( TositeVienti::BRUTTOOIKAISU );
            lisaaKirjausVienti( veroon );
        }
    }
}

bool AlvLaskelma::debetistaKoodilla(int alvkoodi)
{
    return  (( alvkoodi / 100 == 0 || alvkoodi / 100 == 4 ) && alvkoodi % 20 / 10 == 0  ) ||  ( alvkoodi / 100 == 2 )  || alvkoodi ==AlvKoodi::VAHENNYSKELVOTON ;
}

void AlvLaskelma::AlvTaulu::lisaa(const QVariantMap &rivi)
{
    int koodi = rivi.value("alvkoodi").toInt();
    if( !koodit.contains(koodi))
        koodit.insert(koodi, KoodiTaulu());
    koodit[koodi].lisaa(rivi);
}

Euro AlvLaskelma::AlvTaulu::summa(int koodista, int koodiin)
{
    if( !koodiin)
        koodiin = koodista;

    Euro s = 0;
    QMapIterator<int,KoodiTaulu> iter(koodit);
    while( iter.hasNext()) {
        iter.next();
        if( iter.key() >= koodista && iter.key() <= koodiin )
            s += iter.value().summa( debetistaKoodilla( iter.key() ) );
    }
    return s;
}

void AlvLaskelma::KoodiTaulu::lisaa(const QVariantMap &rivi)
{
    int kanta = qRound( rivi.value("alvprosentti").toDouble() * 100 );
    if( !kannat.contains(kanta))
        kannat.insert(kanta, KantaTaulu());
    kannat[kanta].lisaa(rivi);
}

Euro AlvLaskelma::KoodiTaulu::summa(bool debetista) const
{
    Euro s;
    QMapIterator<int,KantaTaulu> iter(kannat);
    while( iter.hasNext()) {
        iter.next();
        s += iter.value().summa(debetista);
    }
    return s;
}

void AlvLaskelma::KantaTaulu::lisaa(const QVariantMap &rivi)
{
    int tili = rivi.value("tili").toInt();
    if( !tilit.contains(tili))
        tilit.insert(tili, TiliTaulu());
    tilit[tili].lisaa(rivi);
}

Euro AlvLaskelma::KantaTaulu::summa(bool debetista) const
{
    Euro s;
    QMapIterator<int,TiliTaulu> iter(tilit);
    while( iter.hasNext()) {
        iter.next();
        s += iter.value().summa(debetista);
    }
    return s;
}

void AlvLaskelma::TiliTaulu::lisaa(const QVariantMap &rivi)
{
    viennit.append(rivi);
}

Euro AlvLaskelma::TiliTaulu::summa(bool debetista) const
{
    Euro s = 0;
    for( auto& vienti : viennit ) {
        if( debetista ) {
            s += Euro::fromVariant(vienti.value("debet"));
            s -= Euro::fromVariant(vienti.value("kredit").toDouble());
        } else {
            s -= Euro::fromVariant( vienti.value("debet").toDouble());
            s += Euro::fromVariant( vienti.value("kredit").toDouble());
        }
    }
    return s;
}
