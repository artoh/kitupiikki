#include "oikeusinfonmuodostaja.h"

QString OikeusInfonMuodostaja::oikeusinfo(const QStringList &oikeudet)
{
    if( instanssi__ == nullptr)
        instanssi__ = new OikeusInfonMuodostaja();

    return instanssi__->muodosta(oikeudet);
}

OikeusInfonMuodostaja::OikeusInfonMuodostaja()
{
    OikeusOtsikko tositteet(tr("Tositteet"));
    tositteet.lisaa("Ts", tr("Selaus"));
    tositteet.lisaa("Tk", tr("Kommentointi"));
    tositteet.lisaa("Tl", tr("Luonnoksen laatiminen"));
    tositteet.lisaa("Tt", tr("Tallentaminen ja muokkaaminen"));
    otsikot_.append(tositteet);

    OikeusOtsikko laskut(tr("Laskut"));
    laskut.lisaa("Ls", tr("Selaaminen"));
    laskut.lisaa("Ll", tr("Laskun laatiminen"));
    laskut.lisaa("Lt", tr("Laskun lähettäminen"));
    laskut.lisaa("Xt", tr("Tuotteiden muokkaminen"));
    laskut.lisaa("Xr", tr("Ryhmien muokkaaminen"));
    otsikot_.append(laskut);

    OikeusOtsikko kierto(tr("Laskujen kierto"));
    kierto.lisaa("Kl", tr("Laskun lisääminen kiertoon"));
    kierto.lisaa("Kt", tr("Laskun tarkastaminen"));
    kierto.lisaa("Kh", tr("Laskun hyväksyminen"));
    kierto.lisaa("Ks", tr("Kaikkien kiertojen selaaminen"));
    otsikot_.append(kierto);

    OikeusOtsikko raportit(tr("Raportit"));
    raportit.lisaa("Ra", tr("Raporttien tulostaminen"));
    otsikot_.append(raportit);

    OikeusOtsikko yllapito(tr("Ylläpito"));
    yllapito.lisaa("Av", tr("Arvonlisäveroilmoitus"));
    yllapito.lisaa("Bm", tr("Budjetin muokkaaminen"));
    yllapito.lisaa("Tp", tr("Tilinpäätöksen tekeminen"));
    yllapito.lisaa("As", tr("Asetuksten muokkaaminen"));
    yllapito.lisaa("Ko", tr("Käyttöoikeuksien myöntäminen"));
    otsikot_.append(yllapito);

    OikeusOtsikko tilitoimisto(tr("Tilitoimisto"));
    tilitoimisto.lisaa("OB", tr("Kirjanpidon luominen"));
    tilitoimisto.lisaa("OD", tr("Kirjanpidon siirtäminen ja poistaminen"));
    tilitoimisto.lisaa("OP", tr("Yksittäisten käyttöoikeuksien myöntäminen"));
    tilitoimisto.lisaa("OT", tr("Tilitoimistonäkymä"));
    otsikot_.append(tilitoimisto);

    OikeusOtsikko hallinta("Hallinta");
    hallinta.lisaa("OM", tr("Käyttäjäryhmien muokkaaminen"));
    hallinta.lisaa("OG", tr("Ryhmien muokkaaminen"));
    hallinta.lisaa("OL", tr("Kirjautumistietojen selaaminen"));
    hallinta.lisaa("OS", tr("Tukikirjautuminen"));
    otsikot_.append(hallinta);

}

QString OikeusInfonMuodostaja::muodosta(const QStringList &oikeudet)
{
    QString ulos;
    for(const auto& item : otsikot_) {
        ulos.append( item.muodosta(oikeudet) );
    }
    return ulos;
}


OikeusInfonMuodostaja* OikeusInfonMuodostaja::instanssi__ = nullptr;

OikeusInfonMuodostaja::OikeusOtsikko::OikeusOtsikko(const QString &nimi) :
    nimi_{nimi}
{

}

void OikeusInfonMuodostaja::OikeusOtsikko::lisaa(const QString &lyhenne, const QString &selite)
{
    lista_.append(qMakePair(lyhenne, selite));
}

QString OikeusInfonMuodostaja::OikeusOtsikko::muodosta(const QStringList &oikeudet) const
{
    QString tulos;
    for(const auto& item : lista_) {
        if(oikeudet.contains(item.first)) {
            tulos.append(QString("<li>%1</li>").arg(item.second));
        }
    }
    if( !tulos.isEmpty())
        return QString("<b>%1</b><ul>%2</ul>").arg(nimi_, tulos);
    else
        return QString();
}
