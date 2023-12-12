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

TilioteKirjausRivi::TilioteKirjausRivi(TilioteModel *model) :
    TilioteRivi(model),
    taydennys_(model->kitsas())
{
    rivit_.append(TilioteAliRivi());
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
    TilioteAliRivi rivi;
    paivamaara_ = tuonti.value("pvm").toDate();

    Euro euro = tuonti.value("euro").toString();
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
    rivi.setBrutto( tyyppi_ == OSTO ? Euro::Zero - euro : euro );
    rivi.setSelite( otsikko_ );
    rivi.setKohdennus( tuonti.value("kohdennus").toInt());
    rivi.setEra( era );
    rivi.setVientiId( tuonti.value("id").toInt() );

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

    rivit_.append(TilioteAliRivi());
}


QVariant TilioteKirjausRivi::riviData(int sarake, int role) const
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
            QSet<int> prossat;
            QSet<int> koodit;
            for(const auto& rivi : rivit_) {
                prossat.insert( (int) rivi.alvprosentti() );
                koodit.insert( rivi.alvkoodi());
            }

            if( prossat.count() == 1 && koodit.count() == 1) {
                Euro vero;
                const int prossa = *prossat.begin();

                if( rivit_.first().naytaBrutto() && rivit_.first().naytaNetto()) {
                    for(const auto& rivi: rivit_) {
                        vero += (rivi.brutto() - rivi.netto());
                    }
                    return QString("%2   %1 %").arg( QString::number(prossa), vero.display(false));
                } else if( prossa ) {
                    return QString("%1 %").arg(prossa);
                } else {
                    return QString();
                }
            } else if( prossat.count() > 1) {
                return "...";
            }
            break;
        }
        case KOHDENNUS:
        {
            if( rivit_.value(0).eraId()) {
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
            }
            return QVariant();
        }
        case EURO: {
            return summa();
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
            return rivit_.value(0).alvkoodi() * 100 +
                   (int) rivit_.value(0).alvprosentti();
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
        if(sarake == KOHDENNUS ) {
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
            } else {
                return QIcon(":/pic/tyhja.png");
            }
        } else if( sarake == EURO) {
            const Euro& eur = summa();
            if( !eur ) return QIcon(":/pic/tyhja.png");

            switch(tyyppi()) {
            case OSTO: return eur < Euro::Zero ? QIcon(":/pic/poista.png") : QIcon(":/pic/edit-undo.png");
            case MYYNTI: return eur > Euro::Zero ? QIcon(":/pic/lisaa.png") : QIcon(":/pic/edit-undo.png");
            case SUORITUS: return QIcon(":/pic/lasku.png");
            case SIIRTO: return QIcon(":/pic/siirra.png");
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
        const QList<int> tilinumerot = kirjausTilit();
        if( tilinumerot.count() == 1) {
            Tili* tili = model()->kitsas()->tilit()->tili(tilinumerot.first());
            if( tili) {
                return tili->onko(TiliLaji::MENO);
            }
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
    case PVM:
        paivamaara_ = value.toDate();
        break;
    case SAAJAMAKSAJA:
        kumppani_ = value.toMap();
        break;
    case TILI: {        
        const Tili* tili = model()->kitsas()->tilit()->tili(value.toInt());
        if( !tili) return false;
        const int tiliKohdennus = tili->luku("kohdennus");                
        for(int i=0; i < rivit_.count(); i++) {
            rivit_[i].setTili(tili->numero());
            if(tiliKohdennus) rivit_[i].setKohdennus(tiliKohdennus);
            rivit_[i].setAlvkoodi(tili->alvlaji());
            rivit_[i].setAlvprosentti(tili->alvprosentti());
        }
        break;
    }
    case ALV: {
        const int prosentti = value.toInt() % 100;
        const int koodi = value.toInt() / 100;
        for(int i=0; i < rivit_.count(); i++) {
                rivit_[i].setAlvkoodi( koodi );
                rivit_[i].setAlvprosentti( prosentti );
            }
        break;
    }
    case KOHDENNUS:
        for(int i=1; i < rivit_.count(); i++)
            rivit_[i].setKohdennus(value.toInt());
        break;
    case SELITE:
        otsikko_ = value.toString();
        break;
    case EURO:
        Euro summa = Euro::fromVariant(value);
        if( rivit_[0].naytaBrutto())
            rivit_[0].setBrutto(tyyppi() == OSTO ? Euro::Zero - summa : summa);
        else
            rivit_[0].setNetto( tyyppi() == OSTO ? Euro::Zero - summa : summa );
        break;
    }
    paivitaTyyppi();
    return true;
}

Qt::ItemFlags TilioteKirjausRivi::riviFlags(int sarake) const
{
    if( sarake == EURO && rivit_.count() != 1)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    const QList<int> tiliNumerot = kirjausTilit();
    const Tili* tili = model()->kitsas()->tilit()->tili(tiliNumerot.value(0));


    if( sarake == KOHDENNUS) {
        if( tiliNumerot.count() > 1 || !tili || tili->onko(TiliLaji::TASE) || !model()->kitsas()->kohdennukset()->kohdennuksia() )
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }


    if( sarake == ALV && ( !tili || tili->onko(TiliLaji::TASE)) )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;


    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}


void TilioteKirjausRivi::paivitaTyyppi()
{
    paivitaTyyppi( rivit_.value(0).era(), rivit_.value(0).tilinumero());
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

    pankki.setTyyppi( tyyppi() + TositeVienti::VASTAKIRJAUS);

    lista.append( pankki);

    for(const auto& rivi: rivit_) {
        lista.append( rivi.viennit(tyyppi(), otsikko(), kumppani(), pvm()));
    }        

    for(const auto& taydennys: taydennys_.viennit(lista)) {
        lista.append(taydennys);
    }

    return lista;

}

void TilioteKirjausRivi::paivitaTyyppi(const EraMap &era, const int tilinumero)
{
    const Tili* tili = model()->kitsas()->tilit()->tili( tilinumero );

    if( era.id() > 0)
        tyyppi_ = SUORITUS;
    else if( !tili)
        tyyppi_ = TUNTEMATON;
    else if( tili->onko(TiliLaji::TASE))
        tyyppi_ = SIIRTO;
    else if( tili->onko(TiliLaji::TULO))
        tyyppi_ = MYYNTI;
    else if( tili->onko(TiliLaji::MENO))
        tyyppi_ = OSTO;
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
    Euro summa;
    for(const auto& rivi : rivit_) {
        summa += rivi.naytaBrutto() ? rivi.brutto() : rivi.netto();
    }
    return tyyppi_ == OSTO ? Euro::Zero - summa : summa;
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
    if( vienti.tyyppi() % 100 == TositeVienti::VASTAKIRJAUS) {
        kumppani_ = vienti.kumppaniMap();
        otsikko_ = vienti.selite();
        paivamaara_ = vienti.pvm();
        arkistotunnus_ = vienti.arkistotunnus();
        ostoPvm_ = vienti.ostopvm();
        vientiId_ = vienti.id();
        viite_ = vienti.viite();
        const int tyyppiPohja = vienti.tyyppi() - TositeVienti::VASTAKIRJAUS;
        switch (tyyppiPohja) {
        case MYYNTI:
            tyyppi_ = MYYNTI;
            break;
        case OSTO:
            tyyppi_ = OSTO;
            break;
        case SIIRTO:
            tyyppi_ = SIIRTO;
            break;
        default:
            tyyppi_ = TUNTEMATON;
        }

    } else if( vienti.tyyppi() % 100 == TositeVienti::KIRJAUS)
        rivit_.append( TilioteAliRivi( vienti) );
    else if( !rivit_.count() ) {
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
        rivit_[ rivit_.count() - 1].setNetonVero(vahennys);
        rivit_[ rivit_.count() - 1 ].setAlvvahennys(true, vienti.id());
    } else if( vienti.alvKoodi() == AlvKoodi::VAHENNYSKELVOTON) {
        rivit_[ rivit_.count() - 1].setVahentamaton( vienti.id());
    } else if( vienti.alvKoodi() == AlvKoodi::MAAHANTUONTI_VERO) {
        rivit_[ rivit_.count() - 1 ].setMaahantuonninAlv(vienti.id());
    }

    paivitaTyyppi();
}

void TilioteKirjausRivi::asetaRivi(int indeksi, TilioteAliRivi rivi)
{
    if( indeksi >= rivit_.count())
        rivit_.append(rivi);
    else
        rivit_[indeksi] = rivi;
}

void TilioteKirjausRivi::asetaRivi(TilioteAliRivi rivi)
{
    rivit_.clear();
    rivit_.append(rivi);
}

void TilioteKirjausRivi::poistaRivi(int indeksi)
{
    rivit_.removeAt(indeksi);
}

void TilioteKirjausRivi::lisaaRivi()
{
    rivit_.append(TilioteAliRivi());
}

void TilioteKirjausRivi::tyhjenna()
{
    rivit_.clear();
    lisaaRivi();
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


