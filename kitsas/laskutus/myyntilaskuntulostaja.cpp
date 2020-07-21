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
#include "myyntilaskuntulostaja.h"
#include "db/kirjanpito.h"
#include "db/tositetyyppimodel.h"
#include "validator/ibanvalidator.h"
#include "laskudialogi.h"
#include "erittelyruudukko.h"

#include "nayukiQR/QrCode.hpp"

#include <QFile>
#include <QTextStream>
#include <QSvgRenderer>

#include <QPainter>
#include <QPagedPaintDevice>

#include <QPdfWriter>
#include <QBuffer>
#include <QApplication>

bool MyyntiLaskunTulostaja::tulosta(const QVariantMap &lasku, QPagedPaintDevice *printer, QPainter *painter, bool kuoreen)
{
    MyyntiLaskunTulostaja tulostaja( lasku );
    tulostaja.tulosta(printer, painter, kuoreen);

    return true;
}

QByteArray MyyntiLaskunTulostaja::pdf(const QVariantMap &lasku, bool ikkunakuoreen)
{
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);
    writer.setPageSize( QPdfWriter::A4);
    writer.setPageMargins( QMarginsF(10,10,10,10), QPageLayout::Millimeter );
    QPainter painter(&writer);

    writer.setCreator(QString("%1 %2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ));
    writer.setTitle( tr("Lasku %1").arg( lasku.value("lasku").toMap().value("numero").toInt()) );
    tulosta(lasku, &writer, &painter, ikkunakuoreen);
    painter.end();

    buffer.close();

    return array;
}

QString MyyntiLaskunTulostaja::virtuaaliviivakoodi(const QVariantMap &lasku)
{
    MyyntiLaskunTulostaja tulostaja(lasku);
    return tulostaja.virtuaaliviivakoodi();
}

QString MyyntiLaskunTulostaja::valeilla(const QString &teksti)
{
    QString palautettava;
    for(int i=0; i < teksti.length(); i++)
    {
        palautettava.append(teksti.at(i));
        if( i % 4 == 3)
            palautettava.append(QChar(' '));
    }
    return palautettava;
}

QString MyyntiLaskunTulostaja::bicIbanilla(const QString &iban)
{
    // Pitää olla suomalainen IBAN
    if( !iban.startsWith("FI"))
        return QString();

    // Elokuun 2017 tilanteen mukaan
    QString tunnus = iban.mid(4);

    if( tunnus.startsWith("405") || tunnus.startsWith("497"))
        return "HELSFIHH";  // Aktia Pankki
    else if( tunnus.startsWith('8') )
        return "DABAFIHH";  // Danske Bank
    else if( tunnus.startsWith("34"))
        return "DABAFIHX";
    else if( tunnus.startsWith("31"))
        return "HANDFIHH";  // Handelsbanken
    else if( tunnus.startsWith('1') || tunnus.startsWith('2'))
        return "NDEAFIHH";  // Nordea
    else if( tunnus.startsWith('5'))
        return "OKOYFIHH";  // Osuuspankit
    else if( tunnus.startsWith("39") || tunnus.startsWith("36"))
        return "SBANFIHH";  // S-Pankki
    else if( tunnus.startsWith('6'))
        return "AABAFI22";  // Ålandsbanken
    else if( tunnus.startsWith("47") )
        return "POPFFI22"; // POP Pankit
    else if( tunnus.startsWith("715") || tunnus.startsWith('4'))
        return "ITELFIHH"; // Säästöpankkiryhmä
    else if( tunnus.startsWith("717"))
        return "BIGKFIH1";
    else if( tunnus.startsWith("713"))
        return "CITIFIHX";
    else if( tunnus.startsWith("37"))
        return "DNBAFIHX";
    else if( tunnus.startsWith("799"))
        return "HOLVFIHH";
    else if( tunnus.startsWith("33"))
        return "ESSEFIHX";
    else if( tunnus.startsWith("38"))
        return "SWEDFIHH";

    // Tuntematon pankkikoodi
    return QString();

}

QDate MyyntiLaskunTulostaja::erapaiva()
{
    QDate erapvm = kp()->paivamaara().addDays( kp()->asetukset()->luku("LaskuMaksuaika",14) );
    while( erapvm.dayOfWeek() > 5 ||
           (erapvm.day()==1 && erapvm.month()==1) ||
           (erapvm.day()==6 && erapvm.month()==1) ||
           (erapvm.day()==1 && erapvm.month()==5) ||
           (erapvm.day()==6 && erapvm.month()==12) ||
           (erapvm.day()>= 24 && erapvm.day() <= 26 && erapvm.month()==12))
        erapvm = erapvm.addDays(1);
    return erapvm;
}

MyyntiLaskunTulostaja::MyyntiLaskunTulostaja(const QVariantMap& map, QObject *parent) :
    QObject(parent), map_(map), rivit_( this, map.value("rivit").toList() ),
    ibanit_( kp()->asetus("LaskuIbanit").split(',') )
{
    alustaKaannos( map_.value("lasku").toMap().value("kieli").toString() );

    laskunSumma_ = ( qRound64( rivit_.yhteensa() * 100.0 ) +
                     qRound64( map.value("lasku").toMap().value("aiempisaldo").toDouble() * 100.0)
                    ) / 100.0;
}

MyyntiLaskunTulostaja::MyyntiLaskunTulostaja(const QString &kieli, QObject *parent) :
    QObject(parent)
{
    alustaKaannos(kieli);
}

void MyyntiLaskunTulostaja::tulosta(QPagedPaintDevice *printer, QPainter *painter, bool kuoreen)
{
    double mm = printer->width() * 1.00 / printer->widthMM();
    qreal marginaali = 0.0;
    painter->resetTransform();

    bool ikkunakuori = kuoreen && kp()->asetukset()->onko("LaskuIkkuna");

    if( !map_.value("lasku").toMap().value("numero").toInt() )
    {
        painter->save();
        painter->setPen( QPen( Qt::lightGray));
        painter->setFont( QFont("FreeSans",60,QFont::Black));
        painter->drawText(QRect( 0, 0, painter->window().width(), painter->window().height() ), Qt::AlignCenter, t("luonnos") );
        painter->restore();
    }
    if( kp()->asetukset()->onko("Harjoitus") && !kp()->asetukset()->onko("Demo") )
    {
        painter->save();
        painter->setPen( QPen(Qt::green));
        painter->setFont( QFont("FreeSans",14, QFont::Black));
        painter->drawText(QRect( 0, 0, painter->window().width(), painter->window().height() ), Qt::AlignTop | Qt::AlignRight, t("harjoitus") );
        painter->restore();
    }


    if( laskunSumma_ > 0.0 && map_.value("lasku").toMap().value("maksutapa") != LaskuDialogi::KATEINEN &&
            kp()->asetukset()->onko("LaskuTilisiirto"))
    {
        painter->translate( 0, painter->window().height() - mm * 95 );
        marginaali += alatunniste(printer, painter) + mm * 95;
        tilisiirto(printer, painter);
    } else {
        painter->translate(0, painter->window().height());
        marginaali += alatunniste(printer, painter);
    }
    painter->resetTransform();

    ylaruudukko(printer, painter, ikkunakuori);

    ErittelyRuudukko erittely( map_.value("rivit").toList() , this);
    erittely.tulostaErittely(printer, painter, marginaali);

    // Maksumuistutuksessa on myös erittely aiemmista laskuista
    double avoinsaldo = map_.value("lasku").toMap().value("aiempisaldo").toDouble();
    if( avoinsaldo > 1e-5) {
        QString teksti = t("aiempisaldo").arg(avoinsaldo,0,'f',2);
        painter->setFont(QFont("FreeSans",10,QFont::Bold));
        painter->drawText(0,0,teksti);
        painter->setFont(QFont("FreeSans",10));
        painter->translate(0, painter->fontMetrics().height());
    }

    for(auto item : map_.value("lasku").toMap().value("aiemmat").toList()) {
        if( painter->transform().dy() + painter->fontMetrics().height() * 5 > painter->window().height() - marginaali)
        {
            painter->drawText(QRectF(0,0,painter->window().width()-10*mm, painter->fontMetrics().height() ), Qt::AlignRight, t("jatkuu"));
            printer->newPage();
            painter->resetTransform();
            marginaali = 0;
        }

        QVariantMap muikkari = item.toMap();
        QVariantMap muikkarilasku = muikkari.value("lasku").toMap();

        QString teksti = t(muikkari.value("tyyppi").toInt() == TositeTyyppi::MAKSUMUISTUTUS ?
                               "aiempimuistutus" :
                               "alkuplasku")
                .arg(muikkarilasku.value("numero").toString())
                .arg(muikkarilasku.value("pvm").toDate().toString("dd.MM.yyyy"))
                .arg(muikkarilasku.value("erapvm").toDate().toString("dd.MM.yyyy"));

        painter->drawText(QRectF(0,0,painter->window().width(), painter->fontMetrics().height()*5),Qt::AlignLeft ,teksti);
        painter->translate(0, painter->fontMetrics().height() * 5);

        ErittelyRuudukko muikkariRuudukko( muikkari.value("rivit").toList(), this);
        if( rivit_.rowCount() )
            muikkariRuudukko.tulostaErittely(printer, painter, marginaali);
    }


}

void MyyntiLaskunTulostaja::ylaruudukko( QPagedPaintDevice *printer, QPainter *painter, bool kaytaIkkunakuorta)
{
    const int TEKSTIPT = 10;
    const int OTSAKEPT = 7;

    double mm = printer->width() * 1.00 / printer->widthMM();
    painter->setPen( QPen(QBrush(Qt::black), mm * 0.2));

    // Lasketaan rivinkorkeus. Tehdään painterin kautta, jotta toimii myös pdf-writerillä
    painter->setFont( QFont("FreeSans",OTSAKEPT) );
    double rk = painter->fontMetrics().height();
    painter->setFont(QFont("FreeSans",TEKSTIPT));
    rk += painter->fontMetrics().height();
    rk += 2 * mm;

    double leveys = painter->window().width();
    QVariantMap lasku = map_.value("lasku").toMap();

    // Kuoren ikkuna
    QRectF ikkuna;
    double keskiviiva = leveys / 2;

    if( kaytaIkkunakuorta)
        ikkuna = QRectF( (kp()->asetukset()->luku("LaskuIkkunaX", 0) - printer->pageLayout().margins(QPageLayout::Millimeter).left()  ) * mm,
                       (kp()->asetukset()->luku("LaskuIkkunaY",0) - printer->pageLayout().margins(QPageLayout::Millimeter).top()) * mm,
                       kp()->asetukset()->luku("LaskuIkkunaLeveys",90) * mm, kp()->asetukset()->luku("LaskuIkkunaKorkeus",30) * mm);
    else
        ikkuna = QRectF( 0, rk * 3, keskiviiva, rk * 3);

    if( ikkuna.x() + ikkuna.width() > keskiviiva )
            keskiviiva = ikkuna.x() + ikkuna.width() + 2 * mm;

    double puoliviiva = keskiviiva + ( leveys - keskiviiva ) / 2;

    QRectF lahettajaAlue = QRectF( 0, 0, keskiviiva, rk * 2.2);

    // Jos käytössä on isoikkunakuori, tulostetaan myös lähettäjän nimi ja osoite sinne
    if( kp()->asetukset()->luku("LaskuIkkunaKorkeus", 35) > 55 && kaytaIkkunakuorta)
    {
        lahettajaAlue = QRectF( ikkuna.x(), ikkuna.y(), ikkuna.width(), 30 * mm);
        ikkuna = QRectF( ikkuna.x(), ikkuna.y() + 30 * mm, ikkuna.width(), ikkuna.height() - 30 * mm);
    }

    // Lähettäjätiedot
    double vasen = 0.0;
    double ylos = 0.0;
    bool logossaNimi = kp()->asetukset()->onko("LogossaNimi") && !kp()->logo().size().isEmpty();

    if( !kp()->logo().isNull() )
    {        
        double logosuhde = (1.0 * kp()->logo().width() ) / kp()->logo().height();
        double maxsuhde = logossaNimi ? (lahettajaAlue.width() - 5*mm) / (12.5 * mm) :
                                        (lahettajaAlue.width() / 2) / (12.5 * mm);
        double skaala = logosuhde > maxsuhde ? maxsuhde : logosuhde ;

        if( logossaNimi) {
            painter->drawImage( QRectF( lahettajaAlue.x()+mm, lahettajaAlue.y()+mm, 12.5 * mm * skaala, 12.5 * mm ),  kp()->logo()  );
            ylos += 12.5 * mm;
        } else {
            painter->drawImage( QRectF( lahettajaAlue.x()+mm, lahettajaAlue.y()+mm, rk*2*skaala, rk*2 ),  kp()->logo()  );
            vasen += rk * 2.2 * skaala;
        }
    }
    painter->setFont(QFont("FreeSans",14));
    double pv = painter->fontMetrics().height();
    if( !logossaNimi ) {
        QString nimi = kp()->asetukset()->asetus("LaskuAputoiminimi").isEmpty() ? kp()->asetukset()->asetus("Nimi") : kp()->asetukset()->asetus("LaskuAputoiminimi") ;
        QRectF lahettajaRect = painter->boundingRect( QRectF( lahettajaAlue.x()+vasen, lahettajaAlue.y(),
                                                       lahettajaAlue.width()-vasen, 20 * mm), Qt::TextWordWrap, nimi );
        painter->drawText(QRectF( lahettajaRect), Qt::AlignLeft | Qt::TextWordWrap, nimi);
        ylos += lahettajaRect.height();
    }

    painter->setFont(QFont("FreeSans",9));

    QString omaOsoite = kp()->asetus("Katuosoite") + "\n" +
            kp()->asetus("Postinumero") + " " + kp()->asetus("Kaupunki");

    QRectF lahettajaosoiteRect = painter->boundingRect( QRectF( lahettajaAlue.x()+vasen, lahettajaAlue.y() + ylos,
                                                       lahettajaAlue.width()-vasen, 20 * mm), Qt::TextWordWrap, omaOsoite );
    painter->drawText(lahettajaosoiteRect, Qt::AlignLeft, omaOsoite );

    // Tulostetaan saajan osoite ikkunaan
    painter->setFont(QFont("FreeSans", TEKSTIPT));
    painter->drawText(ikkuna, Qt::TextWordWrap, map_.value("lasku").toMap().value("osoite").toString() );

    pv += rk ;     // pv = perusviiva

    QString asviite = map_.value("lasku").toMap().value("asviite").toString();
    painter->drawLine( QLineF(keskiviiva, pv, keskiviiva,  asviite.isEmpty() ? pv+2 : pv + 3  ));
    for(int i=-1; asviite.isEmpty() ? i < 3 : i < 2; i++)
        painter->drawLine(QLineF(keskiviiva, pv + i * rk, leveys, pv + i * rk));

    QString ytunnus = map_.value("lasku").toMap().value("alvtunnus").toString();
    if( ytunnus.startsWith("FI")) {
        ytunnus.remove(0,2);
        ytunnus.insert(7,'-');
    }

    painter->drawLine( QLineF(keskiviiva, pv-rk, leveys, pv-rk ));
    if( !ytunnus.isEmpty())
        painter->drawLine( QLineF(puoliviiva, pv, puoliviiva, pv+rk ));

    painter->drawLine(QLineF(puoliviiva, pv-rk, puoliviiva, pv));

    painter->setFont( QFont("FreeSans",OTSAKEPT) );

    painter->drawLine(QLineF(keskiviiva, pv-rk, keskiviiva, pv + 2 * rk));

    painter->drawLine( QLineF(keskiviiva, pv+rk*2, leveys, pv+rk*2));

    if( asviite.isEmpty())    {
        painter->drawLine( QLineF(keskiviiva, pv-rk, keskiviiva, pv+rk*2));
    } else {
        painter->drawLine( QLineF(keskiviiva, pv-rk, keskiviiva, pv+rk*3));
        painter->drawLine(QLineF(keskiviiva, pv + 3 * rk, leveys, pv + 3 * rk));
        painter->drawText(QRectF( keskiviiva + mm, pv + rk * 2 + mm, leveys / 4, rk ), Qt::AlignTop, t("asviite"));
    }

    painter->drawText(QRectF( keskiviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("pvm"));

    int tyyppi = map_.value("tyyppi").toInt();
    if( tyyppi ==  TositeTyyppi::HYVITYSLASKU )
    {
        painter->drawText(QRectF( puoliviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("hyvnro"));
        painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk ), Qt::AlignTop, t("hyvpvm"));
    } else if( tyyppi == TositeTyyppi::MAKSUMUISTUTUS) {
        painter->drawText(QRectF( puoliviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("muistutusnro"));
        painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk ), Qt::AlignTop, t("erapvm"));
    } else {
        painter->drawText(QRectF( puoliviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("lnro"));
        painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk ), Qt::AlignTop, t("toimpvm"));
    }

    if( !ytunnus.isEmpty())
    {
        if( ytunnus.at(0).isNumber())
            painter->drawText(QRectF( puoliviiva + mm, pv + mm, leveys / 4, rk), Qt::AlignTop, t("asytunnus"));
        else
            painter->drawText(QRectF( puoliviiva + mm, pv + mm, leveys / 4, rk), Qt::AlignTop, t("asalvtunnus"));
    }

    painter->drawText(QRectF( keskiviiva + mm, pv + rk + mm, leveys / 4, rk ), Qt::AlignTop, t("huomaika"));

    // Viivästyskorko-laatikko vain, jos viivästyskorko määritelty ja laskulla maksettavaa
    double yhteensa = rivit_.yhteensa();
    double viivkorko = map_.value("lasku").toMap().value("viivkorko").toDouble();

    if( yhteensa > 0.0 &&  viivkorko > 1e-5 && map_.value("lasku").toMap().value("maksutapa").toInt() != LaskuDialogi::KATEINEN )  // TODO: Tämä vakio!!
    {
        painter->drawText(QRectF( puoliviiva + mm, pv + rk + mm, leveys / 4, rk ), Qt::AlignTop, t("viivkorko"));
        painter->drawLine(QLineF(puoliviiva, pv+rk, puoliviiva, pv+rk*2));
    }

    painter->setFont(QFont("FreeSans", TEKSTIPT));

    painter->drawText(QRectF( keskiviiva + mm, pv - rk, leveys / 4, rk-mm ), Qt::AlignBottom, lasku.value("pvm").toDate().toString("dd.MM.yyyy") );
    painter->drawText(QRectF( puoliviiva + mm, pv - rk, leveys / 2, rk-mm ), Qt::AlignBottom, lasku.value("numero").toString() );

    QString toimituslaatikkoon = map_.value("lasku").toMap().value("toimituspvm").toDate().toString("dd.MM.yyyy");
    if( tyyppi == TositeTyyppi::MAKSUMUISTUTUS)
        toimituslaatikkoon = lasku.value("erapvm").toDate().toString("dd.MM.yyyy");
    else if (lasku.value("maksutapa").toInt() == LaskuDialogi::ENNAKKOLASKU)
        toimituslaatikkoon = "XX.XX.XXXX";
    else if( lasku.contains("jaksopvm"))
        toimituslaatikkoon = QString("%1 - %2").arg(lasku.value("toimituspvm").toDate().toString("dd.MM.yyyy"))
                                                .arg(lasku.value("jaksopvm").toDate().toString("dd.MM.yyyy"));
    painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk-mm ), Qt::AlignBottom, toimituslaatikkoon );

    painter->drawText(QRectF( puoliviiva + mm, pv + mm, leveys / 4, rk-mm ), Qt::AlignBottom,  ytunnus );

    int maksutapa = map_.value("lasku").toMap().value("maksutapa").toInt();

    if( maksutapa == LaskuDialogi::KATEINEN )
    {
        painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, leveys / 4, rk-mm ), Qt::AlignBottom,  t("kateinen") );
    }
    else if( tyyppi == TositeTyyppi::HYVITYSLASKU)
    {
        painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, leveys - keskiviiva, rk-mm ), Qt::AlignBottom,  t("hlasku"));
    }
    else
    {
        if( tyyppi == TositeTyyppi::MAKSUMUISTUTUS)
        {
            painter->setFont( QFont("FreeSans", TEKSTIPT+2,QFont::Black));
            painter->drawText(QRectF( keskiviiva + mm, pv - rk*2, leveys - keskiviiva, rk-mm ), Qt::AlignBottom,  t("maksumuistutus") );
            painter->setFont(QFont("FreeSans", TEKSTIPT));
        } else if( maksutapa == LaskuDialogi::ENNAKKOLASKU) {
            painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, (leveys -keskiviiva)/2, rk-mm ), Qt::AlignBottom,  t("ennakkolasku") );
        } else
            painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, (leveys -keskiviiva)/2, rk-mm ), Qt::AlignBottom,  t("laskuotsikko") );

        if( laskunSumma_ > 0.0 &&  viivkorko > 1e-5 )
        {
            painter->drawText(QRectF( puoliviiva + mm, pv + rk, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  QString("%L1 %").arg(viivkorko,0,'f',1) );
        }
    }
    painter->drawText(QRectF( keskiviiva + mm, pv + rk, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  QString("%1 %2").arg(kp()->asetukset()->luku("LaskuHuomautusaika",14)).arg(t("paivaa")) );
    painter->drawText(QRectF( keskiviiva + mm, pv + rk * 2, leveys-keskiviiva, rk-mm ), Qt::AlignBottom,  asviite );

    // Kirjoittamista jatkettaan ruudukon jälkeen - taikka ikkunan, jos se on isompi
    qreal ruutukorkeus = asviite.isEmpty() ? pv + rk * 2 : pv + rk *3;
    qreal ikkunakorkeus = ikkuna.y() + ikkuna.height() + 5 * mm;
    painter->setFont( QFont("FreeSans", 10));


    QString lisatieto;

    // Lisätietoja voisi muotoilla vielä paremmin
    if( tyyppi == TositeTyyppi::HYVITYSLASKU) {
        lisatieto.append( t("hyvitysteksti").arg(map_.value("lasku").toMap().value("alkupNro").toLongLong())
                                            .arg(map_.value("lasku").toMap().value("alkupPvm").toDate().toString("dd.MM.yyyy")));
    }

    QString otsikko = map_.value("lasku").toMap().value("otsikko").toString();
    if( !otsikko.isEmpty() && !lisatieto.isEmpty())
        lisatieto.append("\n\n");
    lisatieto.append(otsikko);
    QString info = map_.value("info").toString();
    if( !lisatieto.isEmpty() && !info.isEmpty())
        lisatieto.append("\n\n");
    lisatieto.append(info);

    QRectF lisatiedotRuudunalle = painter->boundingRect(QRectF(keskiviiva, ruutukorkeus + rk/2, leveys-keskiviiva, painter->window().height()), Qt::TextWordWrap, lisatieto );
    QRectF lisatietoIkkunanalle = painter->boundingRect(QRectF(0, ikkunakorkeus + rk ,painter->window().width(), painter->window().height()), Qt::TextWordWrap, lisatieto );

    if( lisatiedotRuudunalle.bottom() < lisatietoIkkunanalle.bottom())
    {
        painter->drawText( lisatiedotRuudunalle, Qt::TextWordWrap, lisatieto);
        ruutukorkeus += rk/2 + lisatiedotRuudunalle.height();
    }
    else
    {
        painter->drawText( lisatietoIkkunanalle, Qt::TextWordWrap, lisatieto);
        ikkunakorkeus += rk/2 + lisatietoIkkunanalle.height();
    }

    painter->translate(0, ruutukorkeus > ikkunakorkeus ? ruutukorkeus + rk : ikkunakorkeus + rk);

}

QString MyyntiLaskunTulostaja::t(const QString &avain) const
{
    return tekstit_.value(avain, "!" + avain);
}

void MyyntiLaskunTulostaja::tekstiRivinLisays(const QString &rivi, const QString& kieli)
{
    int valinpaikka = rivi.indexOf(' ');
    if( valinpaikka > 2 && rivi.length() > 3) {
        QString avain = rivi.left(valinpaikka);
        if( avain.at(0).isUpper() && avain.at(1).isUpper())
        {
            // Kielen mukainen
            if( kieli == avain.left(2)) {
                QString teksti = rivi.mid(valinpaikka + 1);
                teksti.replace('|','\n');
                tekstit_.insert(avain.mid(2), teksti);
            }
        } else {
            if( !tekstit_.contains(avain)) {
                QString teksti = rivi.mid(valinpaikka + 1);
                teksti.replace('|','\n');
                tekstit_.insert(avain, teksti);
            }
        }
    }
}

QChar MyyntiLaskunTulostaja::code128c(int koodattava) const
{
    if( koodattava < 95)
        return QChar( 32 + koodattava);
    else
        return QChar( 105 + koodattava);
}

void MyyntiLaskunTulostaja::tilisiirto(QPagedPaintDevice *printer, QPainter *painter)
{
    painter->setFont(QFont("FreeSans", 7));
    double mm = printer->width() * 1.00 / printer->widthMM();

    // QR-koodi
    if( kp()->asetukset()->onko("LaskuQR"))
    {
        QByteArray qrTieto = qrSvg();
        if( !qrTieto.isEmpty())
        {
            QSvgRenderer qrr( qrTieto );
            qrr.render( painter, QRectF( ( printer->widthMM() - 35 ) *mm, 5 * mm, 30 * mm, 30 * mm  ) );
        }
    }

    double loppu = painter->window().width();
    double pv = (loppu - 20 * mm) / 2 + 20 * mm;
    double osle = (painter->window().width() - 20 * mm) / 2;
    double viervi = pv + 20 * mm;
    double eurv = (painter->window().width() - 20 * mm) * 3 / 4 + 20 * mm;

    painter->drawText( QRectF(0,0,mm*19,mm*16.9), Qt::AlignRight | Qt::AlignHCenter, t("bst"));
    painter->drawText( QRectF(0, mm*18, mm*19, mm*14.8), Qt::AlignRight | Qt::AlignHCenter, t("bsa"));
    painter->drawText( QRectF(0, mm*32.7, mm*19, mm*20), Qt::AlignRight | Qt::AlignTop, t("bmo"));
    painter->drawText( QRectF(0, mm*51.3, mm*19, mm*10), Qt::AlignRight | Qt::AlignVCenter , t("bak"));
    painter->drawText( QRectF(0, mm*62.3, mm*19, mm*8.5), Qt::AlignRight | Qt::AlignHCenter, t("btl"));
    painter->drawText( QRectF(mm * 22, 0, mm*20, mm*10), Qt::AlignLeft, t("iban"));
    painter->drawText( QRectF(pv + mm * 2, 0, mm*20, mm*10), Qt::AlignLeft, t("bic"));

    painter->drawText( QRectF(pv + 2 * mm, mm*53.8, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, t("bvn"));
    painter->drawText( QRectF(pv + 2 * mm, mm*62.3, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, t("bep"));
    painter->drawText( QRectF(eurv + 2 * mm, mm*62.3, mm*19, mm*8.5), Qt::AlignLeft, t("eur"));

    painter->setFont(QFont("FreeSans",6));
    painter->drawText( QRectF( loppu - mm * 60, mm * 72, mm * 60, mm * 20), Qt::AlignLeft | Qt::TextWordWrap, t("behto") );
    painter->setPen( QPen( QBrush(Qt::black), mm * 0.5));
    painter->drawLine(QLineF(pv,0,pv,mm*69.8));
    painter->drawLine(QLineF(0, mm*16.9, pv, mm*16.9));
    painter->drawLine(QLineF(0, mm*31.7, pv, mm*31.7));
    painter->drawLine(QLineF(mm*20, 0, mm*20, mm*31.7));
    painter->drawLine(QLineF(0, mm*61.3, loppu, mm*61.3));
    painter->drawLine(QLineF(0, mm*69.8, loppu, mm*69.8));
    painter->drawLine(QLineF(pv, mm*52.8, loppu, mm*52.8));
    painter->drawLine(QLineF(pv + mm * 19 , mm*52.8, pv + mm * 19, mm*69.8));
    painter->drawLine(QLineF(eurv, mm*61.3, eurv, mm*69.8));
    painter->drawLine(QLineF(mm*20, mm*61.3, mm*20, mm*69.8));

    painter->setPen( QPen(QBrush(Qt::black), mm * 0.13));
    painter->drawLine( QLineF( mm*22, mm*57.1, pv - 3 * mm, mm*57.1));

    painter->setPen( QPen(QBrush(Qt::black), mm * 0.13, Qt::DashLine));
    painter->drawLine( QLineF( 0, -1 * mm, loppu, -1 * mm));

    painter->setFont(QFont("FreeSans", 10));

    painter->drawText(QRectF( mm*22, mm * 33, osle, mm * 25), Qt::TextWordWrap,  map_.value("lasku").toMap().value("osoite").toString() );

    painter->drawText( QRectF(viervi + 2 * mm, mm * 53.8 ,loppu - viervi - 4*mm, mm*7.5), Qt::AlignLeft | Qt::AlignVCenter, muotoiltuViite() );

    painter->drawText( QRectF(viervi + 2 * mm , mm*62.3, eurv - viervi - 4 * mm , mm*7.5), Qt::AlignLeft | Qt::AlignVCenter, map_.value("lasku").toMap().value("erapvm").toDate().toString("dd.MM.yyyy") );
    painter->drawText( QRectF(eurv, mm*62.3, loppu - eurv - 10 * mm, mm*7.5), Qt::AlignRight | Qt::AlignVCenter, QString("%L1").arg( laskunSumma_ ,0,'f',2) );

    painter->drawText( QRectF(mm*22, mm*17, osle, mm*13), Qt::AlignTop | Qt::TextWordWrap, kp()->asetus("Nimi") + "\n" + kp()->asetus("Osoite")  );

    QString tilinumerot;
    QString bicit;
    for(auto iban : ibanit_) {
        tilinumerot.append(valeilla(iban) + '\n');
        bicit.append( bicIbanilla(iban) + '\n');
    }

    painter->drawText( QRectF(mm*22, 0, osle, mm*17), Qt::AlignVCenter, tilinumerot );
    painter->drawText( QRectF(pv+2*mm, 0, osle, mm*17), Qt::AlignVCenter, bicit );

    painter->save();
    painter->setFont(QFont("FreeSans", 7));
    painter->translate(mm * 2, mm* 60);
    painter->rotate(-90.0);
    painter->drawText(0,0,t("btilis"));
    painter->restore();

    // Viivakoodi
    if( kp()->asetukset()->onko("LaskuViivakoodi"))
    {
        painter->save();

        QFont koodifontti( "code128_XL", 36);
        koodifontti.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
        painter->setFont( koodifontti);
        QString koodi( code128() );
        painter->drawText( QRectF( mm*10, mm*72, mm*100, mm*13), Qt::AlignCenter, koodi  );

        painter->restore();
    }
}

qreal MyyntiLaskunTulostaja::alatunniste(QPagedPaintDevice *printer, QPainter *painter)
{
    painter->save();
    painter->setFont( QFont("FreeSans",10));
    qreal rk = 1.1 * painter->fontMetrics().height();
    painter->translate(0, -4.5 * rk);

    qreal leveys = painter->window().width();
    double mm = printer->width() * 1.00 / printer->widthMM();

    if( !virtuaaliviivakoodi().isEmpty()) {
        painter->translate(0,-1*rk);
        painter->drawText(QRectF(0,0,leveys,rk),t("virtviiv") + " " + virtuaaliviivakoodi());
        painter->translate(0,rk);
    }


    painter->setPen( QPen(QBrush(Qt::black), mm * 0.2));
    painter->drawRect(QRectF(leveys * 4 / 5, 0, leveys / 5, rk * 2));

    painter->setFont( QFont("FreeSans",8));
    painter->drawText(QRectF(leveys * 4 / 5 + mm, 0 + mm, leveys / 5, rk), t("Yhteensa"));
    painter->setFont( QFont("FreeSans", 11,QFont::Bold));

    painter->drawText(QRectF(leveys * 4 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight,QString("%L1 €").arg( laskunSumma_,0,'f',2) );

    if( laskunSumma_ >0 )
    {
        painter->drawRect(QRectF(leveys * 3 / 5, 0, leveys / 5, rk * 2));
        painter->setFont( QFont("FreeSans",8));
        painter->drawText(QRectF(leveys * 3 / 5 + mm, 0 + mm, leveys / 5, rk), t("erapvm"));
        painter->setFont( QFont("FreeSans", 11));


        if( map_.value("lasku").toMap().value("maksutapa").toInt()  == LaskuDialogi::KATEINEN )
        {
            painter->drawText(QRectF(leveys * 3 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight, t("maksettu"));
        }
        else
        {
            painter->drawText(QRectF(leveys * 3 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight, map_.value("lasku").toMap().value("erapvm").toDate().toString("dd.MM.yyyy") );

            painter->drawRect(QRectF(leveys * 2 / 5, 0, leveys / 5, rk * 2));
            painter->drawRect(QRectF(0, 0, leveys * 2 / 5, rk * 2));

            painter->setFont( QFont("FreeSans",8));
            painter->drawText(QRectF(leveys * 2 / 5 + mm, 0 + mm, leveys / 5, rk), t("viitenro"));
            painter->drawText(QRectF(mm, 0 + mm, leveys * 2 / 5, rk), t("iban"));

            painter->setFont( QFont("FreeSans", 11));
            QRectF bicRect = painter->boundingRect(QRectF(0, 0, leveys * 2 / 5 -mm, rk * 2), Qt::AlignBottom | Qt::AlignLeft, bicIbanilla(ibanit_.value(0))+'X');
            qreal bicPaikka = leveys * 2 / 5 - bicRect.width() - mm;

            painter->drawText(QRectF(leveys * 2 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight,  muotoiltuViite() );
            painter->drawText(QRectF(mm, 0, leveys * 2 / 5 -mm, rk * 2), Qt::AlignBottom | Qt::AlignLeft, valeilla(ibanit_.value(0)) );
            painter->drawText(QRectF(bicPaikka, 0, bicPaikka, rk * 2), Qt::AlignBottom | Qt::AlignLeft, bicIbanilla(ibanit_.value(0)) );

            painter->setFont( QFont("FreeSans",8));
            painter->drawText(QRectF(bicPaikka, 0 + mm, leveys * 2 / 5, rk), t("bic"));
            painter->drawLine( bicPaikka - mm, 0, bicPaikka - mm, rk * 2);

            painter->drawRect(QRectF(leveys * 2 / 5, 0, leveys / 5, rk * 2));

        }
    }

    painter->translate(0, 2.1 * rk );
    painter->setFont(QFont("FreeSans",8));
    painter->drawText(QRectF(0,0,leveys/5*2,rk), Qt::AlignLeft, kp()->asetukset()->asetus("Nimi")  );
    painter->drawText(QRectF(0, painter->fontMetrics().height(),leveys/5*2,rk), Qt::AlignLeft, kp()->asetukset()->asetus("Email")  );

    if( !kp()->asetukset()->asetus("Puhelin").isEmpty() )
    {
        painter->drawText(QRectF(leveys/5*2,0,leveys/5,rk), Qt::AlignLeft, t("puhelin"));
        painter->drawText(QRectF(leveys/5*2, painter->fontMetrics().height() ,leveys/5,rk), Qt::AlignLeft, kp()->asetus("Puhelin"));
    }

    QString ytunnus = kp()->asetukset()->asetus("Ytunnus");

    if( !ytunnus.isEmpty() )
    {
        painter->drawText(QRectF(leveys/5*3,0,leveys/5,rk), Qt::AlignLeft, t("ytunnus"));
        painter->drawText(QRectF(leveys/5*3, painter->fontMetrics().height() ,leveys/5,rk), Qt::AlignLeft, ytunnus);
    }
    if( kp()->asetukset()->onko("AlvVelvollinen"))
    {
        painter->drawText(QRectF(leveys/5*4,0,leveys/5,rk), Qt::AlignLeft, t("alvtunnus"));
        painter->drawText(QRectF(leveys*4/5,painter->fontMetrics().height(),leveys/5,rk), Qt::AlignLeft,  "FI"+ytunnus.left(7)+ytunnus.right(1) );
    }
    else
    {
        painter->drawText(QRectF(leveys*4 / 5,0,leveys/5,2*rk), Qt::TextWordWrap , t("eialv") );
    }

    // Viivakoodi
    if( kp()->asetukset()->onko("LaskuViivakoodi") && !kp()->asetukset()->onko("LaskuTilisiirto")
            && laskunSumma_ > 0 && map_.value("lasku").toMap().value("maksutapa").toInt() != LaskuDialogi::KATEINEN )
    {
        QFont koodifontti( "code128_XL", 36);
        koodifontti.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
        painter->setFont( koodifontti);
        QString koodi( code128() );
        painter->drawText( QRectF( mm*20, -2.2*rk-mm*15, mm*100, mm*10), Qt::AlignCenter, koodi  );

        painter->restore();
        return  5.7 * rk + mm * 16;
    }

    painter->restore();
    return 5.5 * rk;
}

QByteArray MyyntiLaskunTulostaja::qrSvg() const
{
    // Esitettävä tieto
    QString data("BCD\n001\n1\nSCT\n");

    QString bic = bicIbanilla(ibanit_.value(0));
    if( bic.isEmpty())
        return QByteArray();
    data.append(bic + "\n");
    data.append(kp()->asetukset()->asetus("Nimi") + "\n");
    data.append(ibanit_.value(0) + "\n");
    data.append( QString("EUR%1\n\n").arg( laskunSumma_, 0, 'f', 2 ));
    data.append(muotoiltuViite().remove(QChar(' ')) + "\n\n");
    data.append( QString("ReqdExctnDt/%1").arg( map_.value("lasku").toMap().value("erapvm").toDate().toString(Qt::ISODate) ));

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText( data.toUtf8().data() , qrcodegen::QrCode::Ecc::QUARTILE);
    return QByteArray::fromStdString( qr.toSvgString(1) );
}

QString MyyntiLaskunTulostaja::muotoiltuViite() const
{

    QString viite = map_.value("lasku").toMap().value("viite").toString();
    if( kp()->asetukset()->onko("LaskuRF"))
    {
        QString rf= "RF00" + viite;
        int tarkiste = 98 - IbanValidator::ibanModulo( rf );
        return valeilla( QString("RF%1%2").arg(tarkiste,2,10,QChar('0')).arg(rf.mid(4)));
    }
    else
        return valeilla(viite);

}

void MyyntiLaskunTulostaja::alustaKaannos(const QString &kieli)
{
    // Ladataan tekstit
    QFile tiedosto(":/lasku/laskutekstit.txt");
    tiedosto.open(QIODevice::ReadOnly);

    QTextStream in(&tiedosto);
    in.setCodec("utf-8");

    while( !in.atEnd())
        tekstiRivinLisays(in.readLine(), kieli);
    for( QString rivi : kp()->asetukset()->lista("LaskuTilit"))
        tekstiRivinLisays( rivi, kieli);
}

QString MyyntiLaskunTulostaja::code128() const
{
    QString koodi;
    koodi.append( QChar(210) );   // Code C aloitusmerkki

    int summa = 105;
    int paino = 1;

    QString koodattava = virtuaaliviivakoodi();
    if( koodattava.length() != 54)  // Pitää olla kelpo virtuaalikoodi
        return QString();

    for(int i = 0; i < koodattava.length(); i = i + 2)
    {
        int luku = koodattava.at(i).digitValue()*10 + koodattava.at(i+1).digitValue();
        koodi.append( code128c(luku) );
        summa += paino * luku;
        paino++;
    }

    koodi.append( code128c( summa % 103 ) );
    koodi.append( QChar(211) );

    return koodi;
}

QString MyyntiLaskunTulostaja::virtuaaliviivakoodi() const
{    
    qlonglong summa = qRound64( laskunSumma_ * 100);

    if( summa <= 0 || summa > 99999999 || map_.value("lasku").toMap().value("maksutapa").toInt() == LaskuDialogi::KATEINEN)  // Ylisuuri tai alipieni laskunsumma
        return QString();
    if( ibanit_.value(0).isEmpty() || !map_.value("lasku").toMap().value("viite").toInt() || !summa )
        return QString();

    QString koodi = kp()->asetukset()->onko("LaskuRF") ?
         QString("5 %1 %2 %3 %4 %5")
                .arg(ibanit_.value(0).mid(2,16))    // Numeerinen tilinumero
                .arg(summa, 8, 10, QChar('0'))
                .arg(muotoiltuViite().mid(2,2) )
                .arg(muotoiltuViite().remove(' ').mid(4),21,QChar('0'))
                .arg( map_.value("lasku").toMap().value("erapvm").toDate().toString("yyMMdd"))
        :
        QString("4 %1 %2 000 %3 %4")
            .arg( ibanit_.value(0).mid(2,16) )  // Tilinumeron numeerinen osuus
            .arg( summa, 8, 10, QChar('0') )  // Rahamäärä
            .arg( map_.value("lasku").toMap().value("viite").toString(), 20, QChar('0'))
            .arg( map_.value("lasku").toMap().value("erapvm").toDate().toString("yyMMdd"));

    return koodi.remove(QChar(' '));
}
