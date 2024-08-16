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
#include "tiliotekirjausrivi.h"

#include "db/kirjanpito.h"
#include "db/kitsasinterface.h"
#include "kirjaus/eurodelegaatti.h"
#include "tiliotemodel.h"
#include "db/tilimodel.h"
#include "db/kohdennusmodel.h"
#include "db/asetusmodel.h"
#include "db/tili.h"
#include "db/verotyyppimodel.h"
#include "db/tositetyyppimodel.h"

#include "model/eramap.h"

#include <QRandomGenerator>
#include <QBrush>
#include <QPalette>
#include <QtGlobal>

TilioteKirjausRivi::TilioteKirjausRivi(TilioteModel *model) :
    TilioteRivi(model),
    taydennys_(model ? model->kitsas() : nullptr)
{
    rivit_.append(ApuriRivi());
}

TilioteKirjausRivi::TilioteKirjausRivi(const QVariantList &data, TilioteModel *model) :
    TilioteRivi(model), taydennys_(model->kitsas())
{
    for(auto const &vienti : data) {
        lisaaVienti( vienti.toMap() );
    }

    paivitaErikoisrivit();
}

TilioteKirjausRivi::TilioteKirjausRivi(const QVariantMap &tuonti, TilioteModel *model) :
    TilioteRivi(model), tuotu_(true), taydennys_(model->kitsas())
{
    ApuriRivi rivi;
    paivamaara_ = tuonti.value("pvm").toDate();

    Euro euro = tuonti.value("euro").toString();
    summa_ = euro;
    int tiliNumero = tuonti.value("tili").toInt();
    if( !tiliNumero ) {
        if(euro > Euro::Zero && model->kitsas()->asetukset()->onko("TilioteTuloKaytossa") ) {
            tiliNumero = model->kitsas()->asetukset()->luku("TilioteTulotili");
        } else if( tuonti.value("ktokoodi").toInt() == 721 && euro < Euro::Zero && model->kitsas()->asetukset()->onko("TiliotePankkikorttitiliKaytossa") ) {
            tiliNumero = model->kitsas()->asetukset()->luku("TiliotePankkikorttitili");
        } else if( euro < Euro::Zero && model->kitsas()->asetukset()->onko("TilioteMenoKaytossa")) {
            tiliNumero = model->kitsas()->asetukset()->luku("TilioteMenotili");
        }
    }

    EraMap era(tuonti.value("era").toMap());
    otsikko_ = tuonti.value("selite").toString();

    paivitaTyyppi( era, tiliNumero);

    rivi.setTili(tiliNumero);


    Tili tili = model->kitsas()->tilit()->tiliNumerolla(tiliNumero);

    // Alv-velvollisuuden huomioiminen tuonnissa
    bool onkoAlv = model->kitsas()->asetukset()->onko(AsetusModel::AlvVelvollinen);

    if( onkoAlv && (tili.onko(TiliLaji::TULO) || tili.onko(TiliLaji::MENO)) ) {
        rivi.setAlvkoodi(tili.alvlaji());
        const double prosentti = tili.alvprosentti() == 24.0 ? yleinenAlv(paivamaara_) / 100.0 : tili.alvprosentti();
        rivi.setAlvprosentti(prosentti);
    } else {
        rivi.setAlvkoodi(AlvKoodi::EIALV);
        rivi.setAlvprosentti(0);
    }

    // Tuonnissa huomioidaan alv-laji (PALAUTE-356)
    if( rivi.naytaBrutto() || rivi.alvkoodi() == AlvKoodi::EIALV)
        rivi.setBrutto(euro.abs());
    else
        rivi.setNetto(euro.abs());

    rivi.setSelite( otsikko_ );
    rivi.setKohdennus( tuonti.value("kohdennus").toInt());
    rivi.setEra( era );

    rivit_.append(rivi);

    QVariantMap kumppani;
    const QString& kumppaniNimi = tuonti.value("saajamaksaja").toString();
    int kumppaniId = tuonti.value("saajamaksajaid").toInt();
    const QString& iban = tuonti.value("iban").toString();

    if( kumppaniId) kumppani.insert("id", kumppaniId);
    if( !kumppaniNimi.isEmpty()) kumppani.insert("nimi", kumppaniNimi);
    if( !iban.isEmpty()) kumppani.insert("iban", QStringList() << iban);

    if( !kumppani.isEmpty()) kumppani_ = kumppani;

    const QString& arkistotunnus = tuonti.value("arkistotunnus").toString();
    arkistotunnus_ =  arkistotunnus.length() > 10 ? arkistotunnus : pseudoarkistotunnus() ;
    viite_ = tuonti.value("viite").toString();
    ostoPvm_ = tuonti.value("ostopvm").toDate();

    paivitaErikoisrivit();
}

TilioteKirjausRivi::TilioteKirjausRivi(const QDate &pvm, TilioteModel *model) :
    TilioteRivi(model), taydennys_(model->kitsas())
{
    paivamaara_ = pvm;
    arkistotunnus_ = pseudoarkistotunnus();

    rivit_.append(ApuriRivi());
}


QVariant TilioteKirjausRivi::riviData(int sarake, int role, const QDate &alkuPvm, const QDate &loppuPvm) const
{            

    switch (role) {
    case Qt::DisplayRole:
    case LajitteluRooli:
        switch (sarake) {
        case PVM:
            if( role == LajitteluRooli) {
                return QString("%1 %2").arg(pvm().toString(Qt::ISODate))
                                        .arg(lisaysIndeksi(),6,10,QChar('0'));
            } else
                return pvm();
        case SAAJAMAKSAJA:
            return kumppani_.value("nimi").toString();
        case SELITE:
            if(otsikko().isEmpty()) {
                if( viite().isEmpty() && ostoPvm().isValid())
                    return QString("[%1]").arg(ostoPvm().toString("dd.MM.yyyy"));
                return viite();
            }
            return otsikko();
        case TILI:
        {
            const QList<int> tilinumerot = kirjausTilit();
            if( tilinumerot.count() == 1) {
                return model()->kitsas()->tilit()->tiliNumerolla(tilinumerot.first()).nimiNumero();
            } else {
                QStringList strlist;
                for(const auto& tilinumero : tilinumerot) {
                    strlist << QString::number(tilinumero);
                }
                return strlist.join(", ");
            }
        }
        case ALV: {
            QSet<double> prossat;
            QSet<int> koodit;
            for(const auto& rivi : rivit_) {
                prossat.insert( rivi.alvprosentti() );
                koodit.insert( rivi.alvkoodi());
            }

            if( prossat.count() == 1 && koodit.count() == 1) {
                Euro vero;
                const double prossa = *prossat.begin();

                if( rivit_.first().naytaBrutto() && rivit_.first().naytaNetto()) {
                    for(const auto& rivi: rivit_) {
                        vero += (rivi.brutto() - rivi.netto());
                    }
                    return QString("%2   %1 %").arg( QString::number(prossa,'f',1), vero.display(false));
                } else if( prossa ) {
                    return QString("%1 %").arg(prossa,0,'f',1);
                } else {
                    return QString();
                }
            } else {
                return "...";
            }
            break;
        }
        case KOHDENNUS:
        {
            if( rivit_.count() > 1) return QVariant();
            const EraMap& era = rivit_.value(0).era();
            if( era.id()) {
                const EraMap& era = rivit_.value(0).era();
                if( era.id() > 0) {
                    return model()->kitsas()->tositeTunnus(era.tunniste(),
                                                           era.pvm(),
                                                           era.sarja());
                } else if (era.id() == EraMap::Uusi) {
                    return TilioteModel::tr("Uusi erä");
                } else if( era.eratyyppi() == EraMap::Huoneisto) {
                    return era.huoneistoNimi();
                } else if( era.eratyyppi() == EraMap::Asiakas) {
                    return era.asiakasNimi();
                }                
            } else {
                const int kohd = kohdennus();
                if(kohd) {
                    return model()->kitsas()->kohdennukset()->kohdennus( kohd ).nimi();
                }
                Tili* tili = model()->kitsas()->tilit()->tili( rivit_.value(0).tilinumero() );
                if( tili && tili->eritellaankoTase()) {
                    return TilioteModel::tr("Ei tase-erää");
                }
            }
            return QVariant();
        }
        case EURO: {
            if( role == LajitteluRooli)
                return summa().cents();
            else
                return summa().display(false);
            }
        default:
            return QVariant();
        }

    case Qt::EditRole:       
        switch (sarake) {
        case PVM:           
            return pvm();
        case TILI:
            return rivit_.value(0).tilinumero();
        case EURO:
            return summa().toDouble();
        case ALV:
            return rivit_.value(0).alvkoodi() +
                   (int) (rivit_.value(0).alvprosentti()*100) * 100;
        case KOHDENNUS:
            return rivit_.value(0).kohdennus();
        case SAAJAMAKSAJA:
            return kumppani();
        case SELITE:
            return otsikko();
        default:
            return QVariant();
        }

    case Qt::UserRole:
        return sarake == SAAJAMAKSAJA ? kumppani_.value("id").toInt() : QVariant();
    case Qt::DecorationRole:
        if(sarake == PVM) {
            if( pvm() < alkuPvm || pvm() > loppuPvm)
                return QIcon(":/pic/varoitus.png");
            else
                return QVariant();
        } else if(sarake == KOHDENNUS ) {
            const int kohd = kohdennus();
            const EraMap& era = rivit_.value(0).era();
            if( kohd) {
                return model()->kitsas()->kohdennukset()->kohdennus( kohd ).tyyppiKuvake();
            } else if( era.eratyyppi() == EraMap::Huoneisto) {
                return QIcon(":/pic/talo.png");
            } else if( era.eratyyppi() == EraMap::Asiakas) {
                return QIcon(":/pic/mies.png"); // Asiakaskohtainen lasku
            } else if( era.id() > 0) {
                return model()->kitsas()->tositeTyypit()->kuvake( era.tositetyyppi() );
            } else if( era.eratyyppi() == EraMap::Uusi) {
                return QIcon(":/pic/lisaa.png");
            }
            Tili* tili = model()->kitsas()->tilit()->tili( rivit_.value(0).tilinumero() );
            if( era.eratyyppi() == EraMap::EiEraa && tili && tili->eritellaankoTase()) {
                return QIcon(":/pic/huomio.png");
            } else {
                return QIcon(":/pic/tyhja.png");
            }
        } else if( sarake == EURO) {
            const Euro& eur = summa();
            if( !eur ) return QIcon(":/pic/tyhja.png");

            switch(tyyppi()) {
            case TositeVienti::OSTO:
            case TositeVienti::MYYNTI:
                return eur > Euro::Zero ? QIcon(":/pic/lisaa.png") : QIcon(":/pic/poista.png");
            case TositeVienti::SUORITUS: return QIcon(":/pic/lasku.png");
            case TositeVienti::SIIRTO:
                return eur > Euro::Zero ? QIcon(":/pic/tilille.png") : QIcon(":/pic/tililta.png");
            default: return QIcon(":/pic/tyhja.png");
            }
        } else if( sarake == ALV) {
            QSet<int> koodit;
            for(const auto& rivi : rivit_) {
                koodit.insert( rivi.alvkoodi());
            }
            if( koodit.count() == 1) {
                return model()->kitsas()->alvTyypit()->kuvakeKoodilla(*koodit.begin());
            } else {
                return QVariant();
            }
        }
        return QVariant();

    case Qt::TextAlignmentRole:
        return sarake == EURO || sarake == ALV ? QVariant(Qt::AlignRight | Qt::AlignVCenter) : QVariant(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::ForegroundRole:
        if( sarake == PVM && otsikko().isEmpty() && kumppani().isEmpty() && kirjausTilit().isEmpty() && !summa())
            return QPalette().placeholderText();
        return (sarake == SELITE && otsikko().isEmpty() ? QBrush(QPalette().base().color().lightness() > 128 ?  QColor(Qt::blue) : QColor(Qt::cyan)) : QPalette().text() );
    case TilaRooli:
        return peitetty() ? "-" : "AA";
    case LisaysIndeksiRooli:
        return lisaysIndeksi();
    case EraIdRooli:
        return rivit_.value(0).eraId();
    case EuroRooli:
        return summa();
    case PvmRooli:
        return pvm();
    case TiliRooli: {
        const QList<int> tilinumerot = kirjausTilit();
        if( tilinumerot.count() == 1) {
            return tilinumerot.first();
        } else {
            return QVariant();
        }
    }
    case EuroDelegaatti::MiinusRooli:
    {
        const Euro& euro = summa();
        if(euro) return euro < Euro::Zero;
        const int tilinumero = rivit_.value(0).tilinumero();
        if( tilinumero ) {
            return model()->kitsas()->tilit()->tiliNumerolla(tilinumero).onko(TiliLaji::MENO);
        }
        return QVariant();
    }
    default:
        return QVariant();
    }

}

bool TilioteKirjausRivi::setRiviData(int sarake, const QVariant &value)
{
    if( riviData(sarake, Qt::EditRole) == value || rivit_.isEmpty() )
        return false;

    switch (sarake) {
    case PVM: {
        const QDate pvm = value.toDate();
        if( pvm <= model()->kitsas()->tilitpaatetty()  ) {
            return false;
        }

        paivamaara_ = value.toDate();
        break;
    }
    case SAAJAMAKSAJA:
        kumppani_ = value.toMap();
        break;
    case TILI: {        
        const Tili* tili = model()->kitsas()->tilit()->tili(value.toInt());
        if( !tili) return false;
        const bool onkoBruttoa = rivit_.at(0).naytaBrutto();
        const int tiliKohdennus = tili->luku("kohdennus");

        rivit_[0].setTili(tili->numero());
        if( tiliKohdennus) rivit_[0].setKohdennus(tiliKohdennus);
        if( model()->kitsas()->asetukset()->onko(AsetusModel::AlvVelvollinen) && tili->onko(TiliLaji::TULOS)) {
            rivit_[0].setAlvkoodi(tili->alvlaji());
            const double prosentti = tili->alvprosentti() == 24.0 ? yleinenAlv(paivamaara_) / 100.0 : tili->alvprosentti();
            rivit_[0].setAlvprosentti(prosentti);
            if( onkoBruttoa && !rivit_.at(0).naytaBrutto()) {
                rivit_[0].setNetto(rivit_.at(0).brutto());
            } else if( !onkoBruttoa && rivit_[0].naytaBrutto()) {
                rivit_[0].setBrutto(rivit_.at(0).netto());
            }
        } else {
            rivit_[0].setAlvkoodi(AlvKoodi::EIALV);
            rivit_[0].setAlvprosentti(0);
        }
        paivitaTyyppi();
        break;
    }
    case ALV: {
        const double prosentti = (value.toInt() / 100) / 100.0;
        const int koodi = value.toInt() % 100;
        rivit_[0].setAlvkoodi(koodi);
        rivit_[0].setAlvprosentti(prosentti);
        for(int i=0; i < rivit_.count(); i++) {
                rivit_[i].setAlvkoodi( koodi );
                rivit_[i].setAlvprosentti( prosentti );
            }
        break;
    }
    case KOHDENNUS:                
        rivit_[0].setKohdennus(value.toInt());
        break;
    case SELITE:
        otsikko_ = value.toString();
        if( rivit_.count() == 1)
            rivit_[0].setSelite(value.toString());
        break;
    case EURO:
        Euro summa = Euro::fromVariant(value);
        summa_ = summa;
        if( rivit_[0].naytaBrutto())
            rivit_[0].setBrutto(summa.abs());
        else
            rivit_[0].setNetto(summa.abs());
        paivitaTyyppi();
        break;
    }
    return true;
}

Qt::ItemFlags TilioteKirjausRivi::riviFlags(int sarake) const
{
    if (sarake == PVM || sarake == SAAJAMAKSAJA || sarake == SELITE)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

    // Monirivisiä on muokattava muokkausikkunan kautta
    if( rivit_.count() != 1)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    const Tili* tili = model()->kitsas()->tilit()->tili(rivit_.at(0).tilinumero());

    if( sarake == KOHDENNUS) {
        if( !tili || tili->onko(TiliLaji::TASE) || !model()->kitsas()->kohdennukset()->kohdennuksia() )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    if( sarake == ALV && ( !tili || tili->onko(TiliLaji::TASE) ) )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}


QVariantList TilioteKirjausRivi::viennit(const int tilinumero) const
{
    QVariantList lista;

    TositeVienti pankki;
    pankki.setPvm( paivamaara_ );
    pankki.setTili( tilinumero );
    pankki.setSelite( otsikko() );
    pankki.setDebet( summa() );
    pankki.setKumppani( kumppani());
    pankki.setArkistotunnus( arkistotunnus() );
    pankki.setViite( viite());
    pankki.setOstoPvm( ostoPvm() );
    pankki.setId( vientiId_ );
    pankki.setTilioteTieto( tilioteLisatieto_);


    pankki.setTyyppi( tyyppi() + TositeVienti::VASTAKIRJAUS);

    lista.append( pankki);

    const bool plusOnKredit = summa() > Euro::Zero;

    for(const auto& rivi: rivit_) {
        lista.append( rivi.viennit(tyyppi(), plusOnKredit, otsikko(), kumppani(), pvm()));
    }        

    for(const auto& taydennys: taydennys_.viennit(lista)) {
        lista.append(taydennys);
    }

    return lista;

}

void TilioteKirjausRivi::paivitaTyyppi() {
    paivitaTyyppi( rivit_.value(0).era(), rivit_.value(0).tilinumero());
}

void TilioteKirjausRivi::paivitaTyyppi(const EraMap &era, const int tilinumero)
{
    const Tili* tili = model()->kitsas()->tilit()->tili( tilinumero );

    if( era.id() > 0)
        tyyppi_ = TositeVienti::SUORITUS;
    else if( !tili)
        tyyppi_ = TositeVienti::TUNTEMATON;
    else if( tili->onko(TiliLaji::TASE))
        tyyppi_ = TositeVienti::SIIRTO;
    else if( tili->onko(TiliLaji::MENO) ) {
        tyyppi_ = TositeVienti::OSTO;
    }
    else if( tili->onko(TiliLaji::TULO))
        tyyppi_ = TositeVienti::MYYNTI;
}

void TilioteKirjausRivi::paivitaErikoisrivit()
{
    const int eraId = rivit_.value(0).eraId();

    if( eraId > 0 && summa() ) {
        if( eraId != taydennys_.eraId() )
            model()->tilaaAlkuperaisTosite( lisaysIndeksi(), eraId );
    }
}

Euro TilioteKirjausRivi::summa() const
{
    return summa_;
}

int TilioteKirjausRivi::kohdennus() const
{
    QSet<int> kohdennukset;
    for(const auto& rivi: rivit_) {
        kohdennukset.insert(rivi.kohdennus());
    }
    if( kohdennukset.count() == 1)
        return *kohdennukset.begin();
    else
        return 0;
}


QList<int> TilioteKirjausRivi::kirjausTilit() const
{
    QList<int> tilit;
    for(const auto& rivi : rivit_) {
        const int tili = rivi.tilinumero();
        if( !tilit.contains(tili))
            tilit.append(tili);
    }
    return tilit;
}


void TilioteKirjausRivi::lisaaVienti(const QVariantMap &map)
{
    TositeVienti vienti(map);

    if( !vienti.tyyppi() && vienti.tili() == model()->tilinumero()) {
        vienti.setTyyppi( TositeVienti::VASTAKIRJAUS );
    }

    if( vienti.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS) {
        kumppani_ = vienti.kumppaniMap();
        otsikko_ = vienti.selite();
        paivamaara_ = vienti.pvm();
        arkistotunnus_ = vienti.arkistotunnus();
        ostoPvm_ = vienti.ostopvm();
        vientiId_ = vienti.id();
        viite_ = vienti.viite();
        tilioteLisatieto_ = vienti.tilioteTieto();
        summa_ = vienti.debetEuro() - vienti.kreditEuro();

        const int tyyppiPohja = vienti.tyyppi() - TositeVienti::VASTAKIRJAUS;

        switch (tyyppiPohja) {
        case TositeVienti::MYYNTI:
            tyyppi_ = TositeVienti::MYYNTI;
            break;
        case TositeVienti::OSTO:
            tyyppi_ = TositeVienti::OSTO;
            break;
        case TositeVienti::SUORITUS:
            tyyppi_ = TositeVienti::SUORITUS;
            break;
        case TositeVienti::SIIRTO:
            tyyppi_ = TositeVienti::SIIRTO;
            break;
        default:
            tyyppi_ = TositeVienti::TUNTEMATON;
        }
        return;

    } else if( vienti.tyyppi() % 100 == TositeVienti::KIRJAUS || (!vienti.tyyppi() && vienti.alvKoodi() < AlvKoodi::ALVKIRJAUS)) {
        vienti.setTyyppi( tyyppi_ + TositeVienti::KIRJAUS );
        rivit_.append( ApuriRivi( vienti, summa_ > Euro::Zero) );
    } else if( !rivit_.count() ) {
        ;
    } else if( !vienti.tyyppi()) {
        if( vienti.alvKoodi() == AlvKoodi::TILITYS)
            taydennys_.asetaDebetId(vienti.id());
        else
            taydennys_.asetaKreditId(vienti.id());
    } else if( vienti.alvKoodi() / 100 == AlvKoodi::ALVKIRJAUS / 100) {
        Euro vero = vienti.kreditEuro() - vienti.debetEuro();
        rivit_[ rivit_.count()-1].setNetonVero(vero, vienti.id());
    } else if( vienti.alvKoodi() / 100 == AlvKoodi::ALVVAHENNYS / 100) {
        Euro vahennys = vienti.debetEuro() - vienti.kreditEuro();
        if( !rivit_[rivit_.count()-1].bruttoSyotetty())
            rivit_[ rivit_.count() - 1].setNetonVero(vahennys);
        rivit_[ rivit_.count() - 1 ].setAlvvahennys(true, vienti.id());
    } else if( vienti.alvKoodi() == AlvKoodi::VAHENNYSKELVOTON) {
        rivit_[ rivit_.count() - 1].setVahentamaton( vienti.id());
    } else if( vienti.alvKoodi() == AlvKoodi::MAAHANTUONTI_VERO) {
        rivit_[ rivit_.count() - 1 ].setMaahantuonninAlv(vienti.id());
    }

    if( tyyppi_ == TositeVienti::TUNTEMATON && vienti.tyyppi() % 100 != TositeVienti::VASTAKIRJAUS) {
        paivitaTyyppi();
    }
}

void TilioteKirjausRivi::asetaRivit(ApuriRivit *rivit)
{
    const Euro summa = rivit->summa();

    rivit_ = rivit->rivit();
    summa_ = rivit->summa();

    if( (summa > Euro::Zero) != rivit->plusOnKredit() ) {
        for(int i=0; i < rivit_.count(); i++)
            rivit_[i].vaihdaEtumerkki();
    }
}


QString TilioteKirjausRivi::pseudoarkistotunnus() const
{
    const QString merkit("abcdefghijklmnopqrstuvwxyz0123456789");

    QString tunnus =QString("@%1-%2-")
            .arg(lisaysIndeksi(),4,10,QChar('0'))
            .arg(QDateTime::currentMSecsSinceEpoch(),12,16,QChar('0'));
    for(int i=0; i<16; ++i)
    {
       int index = QRandomGenerator::global()->bounded(merkit.length()-1);
       QChar nextChar = merkit.at(index);
       tunnus.append(nextChar);
    }
    return tunnus;
}

void TilioteKirjausRivi::alkuperaistositeSaapuu(QVariant *data, int eraId)
{
    const QVariantList lista = data->toMap().value("viennit").toList();
    taydennys_.asetaEra(eraId, lista);

}

void TilioteKirjausRivi::asetaLisaysIndeksi(const int indeksi)
{
    TilioteRivi::asetaLisaysIndeksi(indeksi);
    arkistotunnus_ = pseudoarkistotunnus();
}


