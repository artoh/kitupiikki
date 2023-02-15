#include "pdflaskuntuoja.h"

#include <QPdfDocument>

#include "validator/viitevalidator.h"
#include "validator/ibanvalidator.h"

namespace Tuonti {


PdfLaskunTuoja::PdfLaskunTuoja(PdfTiedosto *tiedosto, TuontiApuInfo info) :
    tiedosto_{tiedosto},
    info_{info}
{

}

const QVariantMap PdfLaskunTuoja::tuo()
{
    if(!tiedosto_->ekasivu()) return QVariantMap();

    haeLomakkeesta();
    haeOtsakkeilla();
    haeTekstista();
    if( kumppaniNimi_.isEmpty())
        haeKumppani();
    if( !euro_ )
       haeEuro();

    return map();
}

bool PdfLaskunTuoja::haeLomakkeesta()
{

    PdfSivu* eka = tiedosto_->ekasivu();

    if(eka->etsiPalat( QStringList() << "TILISIIR" << "GIRER" << "CREDIT TRANSFER", QRect(10,550,40,200)).isEmpty() )
        return false;

    QList<PdfPala*> ibanOtsikkoList = eka->etsiPalat( QStringList() << "IBAN",  QRect(40,500,100,100) );
    if( !ibanOtsikkoList.isEmpty()) {
        PdfPala* otsikkopala = ibanOtsikkoList.last();

        QList<PdfPala*> ibanPalat = eka->etsiPalat(QRect(otsikkopala->vasen(), otsikkopala->yla(), 260, 50));
        QStringList ibanLista = rivitPaloista(ibanPalat, QStringList() << "Saajan" << "tilinumero" << "Mottagarens" << "kontonummer" << "IBAN");
        for(const auto& rivi : ibanLista) {
           lisaaIban(rivi);
        }

        PdfPala* saajaOtsikko = eka->etsiPala("Saaja", QRect(5, otsikkopala->yla() + 20,100,100));
        if( saajaOtsikko && tyyppi_ == MENO) {
            QList<PdfPala*> saajaPalat = eka->etsiPalat(QRect(saajaOtsikko->oikea(), saajaOtsikko->yla() - 10, 290 - saajaOtsikko->oikea(), 44));
            QStringList saajaLista = rivitPaloista(saajaPalat, QStringList() << "Saaja" << "Mottagare" << "Maksajan");
            lisaaKumppani(saajaLista);

        } else if( saajaOtsikko && tyyppi_ == MENO) {
            PdfPala* maksajaOtsikko = eka->etsiPala("Maksajan", QRect(5, saajaOtsikko->ala() + 10, 100, 80));
            if( maksajaOtsikko) {
                QList<PdfPala*> mPalat = eka->etsiPalat( QRect(maksajaOtsikko->oikea(), maksajaOtsikko->yla() - 8, 260, 70));
                QStringList maksajaLista = rivitPaloista(mPalat, QStringList() << "Maksajan" << "nimi" << "ja" << "osoite" << "Betalarens" << "namn" << "och" << "adress"<< "Allekirjoitus" << "Underskift");
                lisaaKumppani(maksajaLista);
            }

        }

    }

    QList<PdfPala*> viiteOtsikkoPalat = eka->etsiPalat( QStringList() << "Viitenumero", QRect(280,650,150,200));
    if( !viiteOtsikkoPalat.empty()) {
        PdfPala* viiteOtsikko = viiteOtsikkoPalat.last();
        QList<PdfPala*> viitePalat = eka->etsiPalat(QRect(viiteOtsikko->oikea(), viiteOtsikko->yla()-4, 260, 24));
        QRegularExpressionMatch viiteMats = viiteRe__.match(palatTekstina(viitePalat));
        if( viiteMats.hasMatch()) {
            QString viite = viiteMats.captured();
            if(ViiteValidator::kelpaako(viite)) {
                viite_ = viite.trimmed();
            }
        }
        PdfPala* eraOtsikko = eka->etsiPala("Eräpäivä", QRect(viiteOtsikko->vasen(), viiteOtsikko->ala(), 50, 50));
        PdfPala* euroOtsikko = eka->etsiPala("Euro", QRect(viiteOtsikko->oikea() + 24, viiteOtsikko->ala(), 300, 50));

        if( eraOtsikko && euroOtsikko) {

            QList<PdfPala*> eraPalat = eka->etsiPalat(QRect(viiteOtsikko->oikea(), viiteOtsikko->yla() - 4, euroOtsikko->vasen() - viiteOtsikko->oikea(), 24));
            QString eraStr = palatTekstina(eraPalat);
            QRegularExpressionMatch eraMats = pvmRe__.match(eraStr);
            if( eraMats.hasMatch()) {
                erapvm_ = QDate::fromString(eraMats.captured(),"dd.M.yyyy");
            }
            QList<PdfPala*> sPalat = eka->etsiPalat(QRect(euroOtsikko->oikea(), euroOtsikko->yla(), 130, 24));
            euro_ = Euro::fromString(palatTekstina(sPalat).remove(pvmRe__).remove(euronSiivous__));
        }
    }


    return true;
}

void PdfLaskunTuoja::haeOtsakkeilla()
{
    if( erapvm_.isNull()){
        QStringList eraPvmList = QStringList() << "eräpäivä" << "eräpvm" << "due date";
        erapvm_ = hae(eraPvmList, Pvm).toDate();
    }

    QStringList laskuPvmList = QStringList() << "laskun pvm" << "päivämäärä" << "laskutettu" << "laskun päiväys";
    laskupvm_ = hae(laskuPvmList, Pvm).toDate();

    QStringList toimitusPvmList = QStringList() << "toimituspäivä" << "toimitettu";
    toimituspvm_ = hae(toimitusPvmList, Pvm).toDate();

    if( viite_.isNull()) {
        QStringList viiteList = QStringList() << "viite" << "ref";
        viite_ = hae(viiteList, Viitenumero).toString();
    }

    QStringList laskuNumeroList = QStringList() << "laskun numero" << "laskunro" << "laskun nro" << "lasku #" << "laskunumero";
    laskunumero_ = hae(laskuNumeroList, Numero).toString();
}

void PdfLaskunTuoja::haeTekstista()
{
    PdfSivu* eka = tiedosto_->ekasivu();
    QString teksti = eka->teksti();

    for(const auto& match : ytunnusRe__.globalMatch(teksti)) {
        QString ehdokas = match.captured();
        if(!info_.omaYtunnus(ehdokas)) {
            kumppaniYtunnus_ = ehdokas;
            break;
        }
    }

    if(iban_.isEmpty()) {
        for(const auto& match : ibanRe__.globalMatch(teksti)) {
            QString ehdokas = match.captured().remove(tyhjaRe__);
            if( info_.omaIban(ehdokas)) {
                tyyppi_ = TULO;
                iban_.clear();
                break;
            }
            if( IbanValidator::kelpaako(ehdokas) && !iban_.contains(ehdokas)) {
                iban_.append(ehdokas);
            }
        }
    }
}

void PdfLaskunTuoja::haeKumppani()
{
    // Haetaan ensimmäisen sivun vasemmasta laidasta
    PdfSivu* eka = tiedosto_->ekasivu();

    QStringList vasenLista;
    for(int i=0; i < eka->riveja(); i++) {
        PdfPala* pala = eka->rivi(i)->pala();

        if( pala && pala->vasen() < 100)
            vasenLista.append(pala->teksti());
    }

    QStringList tunnarit = QStringList() << "oy" << "ry" << "ky" << "ay" << "osk" << "yhtiö" << "ab" << "yhtymä" << "yhdistys";
    QStringList ohita = QStringList() << "lasku" << "pvm" << "päivämäärä" << "tuote" << "kpl";

    for(int r = 0; r < vasenLista.count(); r++) {
        QString ehdokas = vasenLista.at(r);
        if( ehdokas.length() < 5) continue;
        if( info_.omaNimi(ehdokas)) continue;

        if(ehdokas.contains(numeroRe__)) continue;

        for(const auto& ohitus : ohita) {
            if(ehdokas.contains(ohitus)) continue;
        }

        for(const auto& tunnari : tunnarit) {
            if( ehdokas.endsWith(tunnari, Qt::CaseInsensitive)) {
                // NYT NAPPASI ;) OU JEE ;)
                QStringList kumppani = QStringList() << vasenLista.value(r) << vasenLista.value(r+1) << vasenLista.value(r+2);
                lisaaKumppani(kumppani);
                if(!kumppaniOsoite_.isEmpty()) return;
                break;
            }
        }
    }

}

void PdfLaskunTuoja::haeEuro()
{
    // Jos euromäärää ei ole, joudutaan hakemaan sitä epätoivoisesti
    // kaikilla mahdollisilla tempuilla...

    QStringList summaTekstit = QStringList() << "yhteensä" << "verollinen" << "maksettava" << "total" << "yht" << "summa";

    Euro summa;

    QList<PdfPala*> palat = tiedosto_->etsiPalat(summaTekstit);
    for(auto pala : palat) {
        QString palaTeksti = pala->teksti();
        QRegularExpressionMatch palaMats = rahaRe__.match(palaTeksti);
        if( palaMats.hasMatch()) {
            Euro palaEuro = Euro::fromString(palaMats.captured());
            if( palaEuro > summa) summa = palaEuro;
        }

        if( pala->seuraava()) {
            QString sTeksti = pala->seuraava()->teksti();
            QRegularExpressionMatch sMats = rahaRe__.match(sTeksti);
            if( sMats.hasMatch()) {
                Euro sEuro = Euro::fromString(sMats.captured());
                if( sEuro > summa - Euro("0.10")) summa = sEuro;
            }
        }

        if( pala->alapala()) {
            QString aTeksti = pala->alapala()->teksti();
            QRegularExpressionMatch aMats = rahaRe__.match(aTeksti);
            if( aMats.hasMatch()) {
                Euro aEuro = Euro::fromString(aMats.captured());
                if( aEuro > summa - Euro("0.10")) summa = aEuro;
            }
        }
    }

    if(euro_) return;

    QString kokoTeksti = tiedosto_->kokoTeksti();

    for(const auto& match : rahaRe__.globalMatch(kokoTeksti)) {
        QString ehdokas = match.captured();
        Euro maara = Euro::fromString(ehdokas);
        if( maara > euro_- Euro("0.10")) euro_ = maara;
    }
}

QVariant PdfLaskunTuoja::hae(const QStringList &tekstit, Muoto muoto)
{
    PdfSivu* eka = tiedosto_->ekasivu();
    if(!eka) return QVariant();

    QList<PdfPala*> palat = eka->etsiPalat(tekstit);
    for( auto pala : palat) {
        if( pala->seuraava()) {
            QVariant stark = tarkastaMuoto(pala->seuraava(), muoto);
            if(!stark.isNull()) return stark;
        }
        if( pala->alapala()) {
            QVariant staa = tarkastaMuoto(pala->alapala(), muoto);
            if( !staa.isNull()) return staa;
        }
        // Tarkastetaan myös palaa itseään
        QStringList osat = pala->teksti().split(tyhjaRe__);
        if( osat.count() > 1) {
            QString vika = osat.last();
            if( muoto == Pvm) {
                QDate pvm = QDate::fromString(vika, "dd.M.yyyy");
                if(pvm.isValid()) return pvm;
            } else if( muoto == Numero) {
                if(numeroRe__.match(vika).hasMatch()) return vika;
            }
        }
    }
    return QVariant();
}

QVariant PdfLaskunTuoja::tarkastaMuoto(PdfPala* pala, Muoto muoto)
{
    if( muoto == Pvm) {
        QDate pvm = QDate::fromString(pala->teksti(),"dd.M.yyyy");
        if( pvm.isValid()) return pvm;
    } else if( muoto == Viitenumero) {
        QString teksti = pala->teksti();
        while(!ViiteValidator::kelpaako(teksti)) {
            if( !pala->seuraava()) return QVariant();
            pala = pala->seuraava();
            teksti.append(pala->teksti());
        }
        return teksti.remove(QRegularExpression(tyhjaRe__));
    } else if( muoto == Numero) {
        QString teksti = pala->teksti();
        if(numeroRe__.match(teksti).hasMatch()) return teksti;
    }
    return QVariant();
}

QStringList PdfLaskunTuoja::rivitPaloista(QList<PdfPala *> palat, const QStringList &siivottavat)
{
    if( palat.isEmpty()) return QStringList();

    QStringList ulos;
    QString rivi;

    PdfPala* edellinen = palat.value(0);
    for(auto pala : palat) {
        if( pala->yla() > edellinen->ala() + 20) break;
        if( qAbs(pala->vasen() < edellinen->vasen() ) ) {
            if(!rivi.isEmpty()) ulos.append(rivi.trimmed());
            rivi.clear();
        }

        QString teksti = pala->teksti();
        if( rivi.isEmpty()) {
            for(const auto& fraasi : siivottavat) {
                if( teksti == fraasi) {
                    teksti.clear();
                }
                if(teksti.startsWith(fraasi)) {
                    teksti = teksti.mid(fraasi.length()).trimmed();
                }
            }
        }
        if(!teksti.isEmpty()) rivi.append( teksti + " " );
        edellinen = pala;
    }
    if(!rivi.isEmpty()) ulos.append(rivi.trimmed());
    return ulos;
}

QString PdfLaskunTuoja::palatTekstina(QList<PdfPala *> palat)
{
    QString ulos;
    for(auto pala : palat) {
        ulos.append(pala->teksti() + " ");
    }
    return ulos;
}

void PdfLaskunTuoja::lisaaIban(const QString &iban)
{
    QRegularExpressionMatch mats = ibanRe__.match(iban);
    if( mats.hasMatch()) {
        QString ibanTxt = mats.captured().remove(tyhjaRe__);
        if( IbanValidator::kelpaako(ibanTxt)) {
            if( info_.omaIban(ibanTxt)) {
                tyyppi_ = TULO;
            } else {
                iban_.append(ibanTxt);
            }
        }
    }
}

bool PdfLaskunTuoja::lisaaKumppani(const QStringList &rivit)
{
    QString nimi = rivit.value(0);
    if( info_.omaNimi(nimi)) return false;

    kumppaniNimi_ = nimi;

    QStringList kadut = QStringList() << "tie" << "katu" << "kuja" << "PL" << "väylä" << "raitti" << "aukio";
    QString osoite = rivit.value(1);

    for(const auto& katu : kadut) {
        if( osoite.contains(katu, Qt::CaseInsensitive)) {
            kumppaniOsoite_ = osoite;
            break;
        }
    }

    QRegularExpressionMatch mats = postiOsoiteRe__.match(rivit.value(2));
    if(mats.hasMatch()) {
        kumppaniPostinumero_ = mats.captured("nro");
        kumppaniKaupunki_ = mats.captured("kaupunki");
    }

    return true;
}

QVariantMap PdfLaskunTuoja::map() const
{
    QVariantMap data;
    data.insert("tyyppi", tyyppi_);
    if(!iban_.isEmpty()) {
        data.insert("iban", iban_);
    }
    if(erapvm_.isValid())
        data.insert("erapvm", erapvm_);
    if(laskupvm_.isValid())
        data.insert("tositepvm", laskupvm_);
    if(!viite_.isEmpty())
        data.insert("viite", viite_);
    if( euro_)
        data.insert("summa", euro_);
    if( !laskunumero_.isEmpty())
        data.insert("laskunnumero", laskunumero_);

    if( !kumppaniNimi_.isEmpty())
        data.insert("kumppaninimi", kumppaniNimi_);
    if( !kumppaniOsoite_.isEmpty())
        data.insert("kumppaniosoite", kumppaniOsoite_);
    if( !kumppaniKaupunki_.isEmpty())
        data.insert("kumppanikaupunki", kumppaniKaupunki_);
    if( !kumppaniPostinumero_.isEmpty())
        data.insert("kumppanipostinumero", kumppaniPostinumero_);
    if( !kumppaniYtunnus_.isEmpty())
        data.insert("kumppaniytunnus", kumppaniYtunnus_);


    return data;
}

QRegularExpression PdfLaskunTuoja::tyhjaRe__("\\s");
QRegularExpression PdfLaskunTuoja::ibanRe__("\\bFI\\d{2}[\\w\\s]{6,30}\\b");
QRegularExpression PdfLaskunTuoja::postiOsoiteRe__("(?<nro>\\d{5})\\s(?<kaupunki>\\w+)");
QRegularExpression PdfLaskunTuoja::pvmRe__("[0-3]?[0-9][./-][01]?[0-9][./-]20[0-9][0-9]");
QRegularExpression PdfLaskunTuoja::euronSiivous__("[^0-9,.]");
QRegularExpression PdfLaskunTuoja::viiteRe__("(RF)?(\\s|\\d){4,30}");
QRegularExpression PdfLaskunTuoja::numeroRe__("\\d+");
QRegularExpression PdfLaskunTuoja::ytunnusRe__("\\d{7}-\\d");
QRegularExpression PdfLaskunTuoja::rahaRe__("(\\b\\d{1,3}\\s?)*\\d{1,3}[,.]\\d{2}\\b");

} // namespace Tuonti
