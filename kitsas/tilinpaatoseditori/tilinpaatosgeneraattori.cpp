#include "tilinpaatosgeneraattori.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#include <QDebug>

TilinpaatosGeneraattori::TilinpaatosGeneraattori(const Tilikausi &tilikausi, QObject *parent)
    :   QObject{parent}, tilikausi_{tilikausi}
{

}

void TilinpaatosGeneraattori::generoi()
{
    tilaaSaldot();
}

QString TilinpaatosGeneraattori::html() const
{
    return html_;
}

QStringList TilinpaatosGeneraattori::raportit() const
{
    return raportit_;
}

void TilinpaatosGeneraattori::tilaaSaldot()
{
    tilaaSaldo(Nykyinen);
    tilaaSaldo(NykyinenAlku);
    tilaaSaldo(EdellinenAlku);
}

void TilinpaatosGeneraattori::tilaaSaldo(SaldoTyyppi tyyppi)
{
    Tilikausi kausi = tyyppi == EdellinenAlku ? kp()->tilikaudet()->tilikausiPaivalle(tilikausi_.alkaa().addDays(-1)) : tilikausi_;
    if( !kausi.alkaa().isValid()) return;
    tilattuja_++;

    KpKysely* saldoKysely = kpk("/saldot");
    saldoKysely->lisaaAttribuutti("pvm", tyyppi == Nykyinen ? tilikausi_.paattyy() : tilikausi_.alkaa());
    saldoKysely->lisaaAttribuutti("tase");
    if( tyyppi != Nykyinen) {
        saldoKysely->lisaaAttribuutti("alkusaldot");
    }
    connect( saldoKysely, &KpKysely::vastaus, this, [this, tyyppi] (QVariant* data) { this->saldotSaapuu(tyyppi, data); });
    saldoKysely->kysy();
}

void TilinpaatosGeneraattori::saldotSaapuu(SaldoTyyppi tyyppi, QVariant *data)
{
    QVariantMap map = data->toMap();
    QMapIterator<QString, QVariant> iter(map);
    while(iter.hasNext()) {
        iter.next();
        const QString& tili = iter.key();
        const Euro saldo = Euro(iter.value().toString());
        TilinSaldot saldot = saldot_.value(tili);
        saldot.setSaldo(tyyppi, saldo);
        saldot_.insert(tili, saldot);
    }

    tilattuja_--;
    if( tilattuja_ <= 0)
        jatka();
}

void TilinpaatosGeneraattori::jatka()
{
    html_.clear();
    raportit_.clear();
    muuttujat_.clear();

    QStringList pohja =  kp()->asetukset()->lista("tppohja/" + kp()->asetukset()->asetus(AsetusModel::TilinpaatosKieli));
    QStringList valinnat = kp()->asetukset()->asetus(AsetusModel::TilinpaatosValinnat).split(",");

    bool tulosta = true;
    bool atEhto = true;
    bool kyssariEhto = true;

    for( const QString& rivi : pohja) {
        // #-alkuiset rivit (valintarivit)
        QRegularExpressionMatch tunnisteMatch = tunnisteRe__.match(rivi);
        if( tunnisteMatch.hasMatch()) {
            atEhto = true;
            // Tulostetaan, jos haluttu tunniste on valittu (tai ei-ehto ja ei-valittu)
            QString tunniste = tunnisteMatch.captured("t");
            tulosta = tunniste.isEmpty() || valinnat.contains(tunniste);
            // Kuitenkin jos yksikin pois-ehto voimassa niin ei tulosteta
            QStringList pois = tunnisteMatch.captured("p").split(" ", Qt::SkipEmptyParts);
            for( const QString& poistettava : pois) {
                if( valinnat.contains(poistettava.mid(1))) {
                    tulosta = false;
                    break;
                }
            }            
        } else if( rivi.startsWith("?")) {
            kyssariEhto = kyssariTesti(rivi.mid(1));
        } else if( rivi.startsWith("@?") && tulosta && kyssariEhto) {
            atEhto = ehto(rivi.mid(2));
        } else if( rivi.startsWith('@') && tulosta && kyssariEhto) {
            atRivi(rivi);
        } else if( tulosta && atEhto && kyssariEhto) {
            tekstiRivi(rivi);
        }
    }

    emit valmis();
}

void TilinpaatosGeneraattori::tekstiRivi(const QString &rivi)
{
    int position = 0;
    int fieldPosition = rivi.indexOf("{{");

    while( fieldPosition > 0 ) {
        html_.append(rivi.mid(position, fieldPosition - position));
        int fieldEnd = rivi.indexOf("}}", fieldPosition);
        QString field = rivi.mid(fieldPosition + 2, fieldEnd - fieldPosition - 2);

        if( field == "tulos") {
            html_.append(Euro::fromCents(tilikausi_.tulos()).display(true));
        } else if( field == "kausi") {
            html_.append(tilikausi_.kausivaliTekstina());
        } else if( field == "edkausi") {
            const Tilikausi edellinen = kp()->tilikausiPaivalle(tilikausi_.alkaa().addDays(-1));
            html_.append( edellinen.kausivaliTekstina());
        } else if( field == "loppupvm") {
            html_.append(tilikausi_.paattyy().toString("dd.MM."));
        } else if( field == "alkupvm") {
            html_.append(tilikausi_.alkaa().toString("dd.MM."));
        } else if( field == "kausi.loppupvm") {
            html_.append(tilikausi_.paattyy().toString("dd.MM.yyyy"));
        } else if( field == "kausi.alkupvm") {
            html_.append(tilikausi_.alkaa().toString("dd.MM.YYYY"));
        } else if( field == "kotipaikka") {
            html_.append(kp()->asetukset()->asetus(AsetusModel::Kotipaikka));
        } else if( field == "edkausi.loppupvm") {
            const Tilikausi edellinen = kp()->tilikausiPaivalle(tilikausi_.alkaa().addDays(-1));
            html_.append( edellinen.paattyy().toString("dd.MM.yyyy"));
        } else if( field == "edkausi.alkupvm") {
            const Tilikausi edellinen = kp()->tilikausiPaivalle(tilikausi_.alkaa().addDays(-1));
            html_.append( edellinen.alkaa().toString("dd.MM.yyyy"));
        } else if( field == "kayttaja.nimi" && kp()->pilvi()) {
            html_.append( kp()->pilvi()->kayttaja().nimi());
        } else if( field == "pvm") {
            html_.append( QDate::currentDate().toString("dd.MM.yyyy"));
        } else {
            html_.append( laskenta(field).display(true) );
        }

        position = fieldEnd + 2;
        fieldPosition = rivi.indexOf("{{", position);
    }

    html_.append( rivi.mid(position) );
}

bool TilinpaatosGeneraattori::ehto(const QString ehto)
{
    QRegularExpressionMatch match = ehtoRe__.match(ehto);
    if( match.hasMatch()) {
        const Euro vasen = laskenta(match.captured("v"));
        const Euro oikea = laskenta(match.captured("o"));
        const QString& oper = match.captured("e");
        if( oper == "<") return vasen < oikea;
        else if( oper == "<=") return vasen < oikea || vasen == oikea;
        else if( oper == "=") return vasen == oikea;
        else if( oper == ">=") return vasen > oikea || vasen == oikea;
        else if( oper == ">") return vasen > oikea;
        else if( oper == "<>") return vasen != oikea;
        else return false;
    } else if( ehto.isEmpty() ){
        return true;
    } else {
        QStringList splitted = ehto.split(' ', Qt::SkipEmptyParts);
        for(const QString& piece : splitted) {
            if( laskenta(piece) > Euro::Zero)
                return true;
        }
        return false;
    }
}

bool TilinpaatosGeneraattori::kyssariTesti(const QString &ehto)
{
    QRegularExpressionMatch match = ehtoRe__.match(ehto);
    if( match.hasMatch()) {
        const QString avain = match.captured("v");
        const QString vertailu = match.captured("o");
        const QString& oper = match.captured("e");

        const QString asetusArvo = kp()->asetukset()->asetus(avain);
        if( oper == "=") return asetusArvo == vertailu;
    }
    return true;

}

void TilinpaatosGeneraattori::atRivi(const QString &rivi)
{
    if( rivi.startsWith("@henkilosto@")) {
        html_.append(henkilostoTaulukko(rivi.mid(12)));
    } else if( rivi.startsWith("@:") && rivi.contains(' ')) {
        const int space = rivi.indexOf(' ');
        const QString& muuttuja = rivi.mid(2, space - 2);
        const QString& kaava = rivi.mid(space + 1);
        muuttujat_.insert(muuttuja, laskenta(kaava));
    } else {
        QRegularExpressionMatch raporttiMatch = raporttiRe__.match(rivi);
        if( raporttiMatch.hasMatch()) {
            raportit_.append(raporttiMatch.captured());
        }
    }
}

QString TilinpaatosGeneraattori::henkilostoTaulukko(const QString &teksti)
{
    Tilikausi verrokki;
    if( kp()->tilikaudet()->indeksiPaivalle( tilikausi_.paattyy()))
        verrokki = kp()->tilikaudet()->tilikausiIndeksilla(  kp()->tilikaudet()->indeksiPaivalle(tilikausi_.paattyy()) - 1 );

    QString txt = tr("<table width=100%><tr><td width=\"50%\"></td><th align=right width=\"25%\">%1</th>").arg(tilikausi_.kausivaliTekstina());
    if( verrokki.alkaa().isValid() )
        txt.append( QString("<th align=right width=\"25%\">%1</th>").arg(verrokki.kausivaliTekstina()) );

    txt.append(tr("</tr><tr><td>%1</td><td align=right>%2</td>").arg(teksti).arg( kp()->tilikaudet()->tilikausiPaivalle( tilikausi_.paattyy() ).henkilosto()));
    if( verrokki.alkaa().isValid())
        txt.append( QString("<td align=right>%1</td>").arg(verrokki.henkilosto()));
    txt.append("</tr></table>");
    return txt;
}

Euro TilinpaatosGeneraattori::laskenta(const QString &kaava)
{
    Euro summa;
    qDebug() << "Kaava " << kaava;

    QStringList splitted = kaava.split(' ', Qt::SkipEmptyParts);
    for(const QString& part : splitted) {
        if( part.startsWith(':')) {
            summa += muuttujat_.value(part.mid(1), Euro::Zero);
        }

        QRegularExpressionMatch match = kaavaRe__.match(part);
        if( match.hasMatch()) {
            bool minus = match.captured("m") == '-';
            const QString type = match.captured("t");
            const QString start = match.captured("a");
            QString end = match.captured("l");
            if( end.isEmpty()) end = start;

            const int startLenght = start.length();
            const int endLength = end.length();

            QMapIterator<QString, TilinSaldot> iter(saldot_);
            while( iter.hasNext()) {
                iter.next();
                const QString& tili = iter.key();

                if( tili.left(startLenght) >= start &&
                    tili.left(endLength) <= end) {
                    const TilinSaldot& saldot = iter.value();
                    Euro value = saldot.saldo(type);

                    qDebug() << tili << " " <<value.display(true);

                    if( minus ) {
                        summa -= value;
                    } else {
                        summa += value;
                    }
                } else {
                    qDebug() << tili << " --- " << start << " .. " << end;
                }
            }
        } else if( Euro(part)) {
            summa += Euro(part);
        }
    }
    return summa;
}

TilinpaatosGeneraattori::TilinSaldot::TilinSaldot()
{

}

void TilinpaatosGeneraattori::TilinSaldot::setSaldo(SaldoTyyppi tyyppi, Euro saldo)
{
    switch (tyyppi) {
    case Nykyinen:
        saldo_ = saldo;
        break;
    case NykyinenAlku:
        alkusaldo_ = saldo;
        break;
    case EdellinenAlku:
        edellinenAlku_ = saldo;
        break;
    }
}

Euro TilinpaatosGeneraattori::TilinSaldot::saldo(const QString &tyyppi) const
{
    if( tyyppi == "e") return saldo_;
    else if( tyyppi == "s") return alkusaldo_;
    else if( tyyppi == "d") return saldo_ - alkusaldo_;
    else if( tyyppi == "E") return alkusaldo_;
    else if( tyyppi == "S") return edellinenAlku_;
    else if( tyyppi == "D") return alkusaldo_ - edellinenAlku_;
    else return Euro::Zero;
}



QRegularExpression TilinpaatosGeneraattori::tunnisteRe__ = QRegularExpression(R"(^#(?<t>\w+)?\s?(?<p>([-]\w+\s?)*).*$)", QRegularExpression::UseUnicodePropertiesOption);
QRegularExpression TilinpaatosGeneraattori::raporttiRe__ = QRegularExpression("@(.+)(:\\w*)?[!](.+)@", QRegularExpression::UseUnicodePropertiesOption);
QRegularExpression TilinpaatosGeneraattori::kaavaRe__ = QRegularExpression(R"((?<m>[-])?(?<t>[edsEDS])(?<a>\d{1,8})(\.\.(?<l>\d{1,8}))?)", QRegularExpression::UseUnicodePropertiesOption);
QRegularExpression TilinpaatosGeneraattori::ehtoRe__ = QRegularExpression(R"((?<v>.+)(?<e>(<|<=|=|>=|>|<>))(?<o>.+))", QRegularExpression::UseUnicodePropertiesOption);
