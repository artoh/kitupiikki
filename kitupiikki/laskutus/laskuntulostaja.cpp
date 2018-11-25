/*
   Copyright (C) 2017 Arto Hyvättinen

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

#include <QDebug>
#include <cmath>
#include <QSvgRenderer>
#include <QPdfWriter>

#include <QApplication>
#include <QFile>
#include <QTextStream>

#include "laskuntulostaja.h"
#include "db/kirjanpito.h"
#include "laskumodel.h"
#include "nayukiQR/QrCode.hpp"

#include "erittelyruudukko.h"


LaskunTulostaja::LaskunTulostaja(LaskuModel *model, const QString &kieli) : QObject(model), model_(model), kieli_(kieli)
{
    // Hakee laskulle tulostuvan IBAN-numeron
    iban = kp()->tilit()->tiliNumerolla( kp()->asetukset()->luku("LaskuTili")).json()->str("IBAN");

    // Hakee tekstit
    QFile tiedosto(":/lasku/laskutekstit.txt");
    tiedosto.open(QIODevice::ReadOnly);

    QTextStream in( &tiedosto );
    in.setCodec("utf-8");
    while( !in.atEnd() )
    {
        QString rivi = in.readLine();
        rivi.replace('|','\n');
        int valinpaikka = rivi.indexOf(' ');
        if( valinpaikka )
            tekstit_.insert( rivi.left(valinpaikka), rivi.mid(valinpaikka + 1));
    }

    // Vielä muokatut asetuksista
    // Näin voidaan määritellä muokatut tekstit eri kirjanpidoille

    QStringList muokatut = kp()->asetukset()->lista("LaskuTekstit");
    for( QString rivi : muokatut)
    {
        int valinpaikka = rivi.indexOf(' ');
        if( valinpaikka )
            tekstit_[ rivi.left(valinpaikka)] = rivi.mid(valinpaikka + 1);
    }

}

bool LaskunTulostaja::tulosta(QPagedPaintDevice *printer, QPainter *painter, bool kaytaIkkunakuorta)
{
    double mm = printer->width() * 1.00 / printer->widthMM();
    qreal marginaali = 0.0;

    if( kp()->asetukset()->onko("Harjoitus") && !kp()->asetukset()->onko("Demo") )
    {
        painter->save();
        painter->setPen( QPen(Qt::green));
        painter->setFont( QFont("Sans",14));
        painter->drawText(QRect( 0, 0, painter->window().width(), painter->window().height() ), Qt::AlignTop | Qt::AlignRight, QString("HARJOITUS") );
        painter->restore();
    }

    if( model_->laskunSumma() > 0 && model_->kirjausperuste() != LaskuModel::KATEISLASKU && !kp()->asetukset()->onko("LaskuEiTilisiirto"))
    {
        painter->translate( 0, painter->window().height() - mm * 95 );
        marginaali += alatunniste(printer, painter) + mm * 95;
        tilisiirto(printer, painter);
        painter->resetTransform();
    }
    else
    {
        painter->translate(0, painter->window().height());
        marginaali += alatunniste(printer, painter);
    }
    painter->resetTransform();

    ylaruudukko(printer, painter, kaytaIkkunakuorta);
    erittely( model_, printer, painter, marginaali);

    painter->translate(0, mm*15);

    if( model_->tyyppi() == LaskuModel::MAKSUMUISTUTUS)
    {
        QString maksutieto = t("alkuplasku")
                   .arg( model_->viittausLasku().viite)
                   .arg( model_->viittausLasku().pvm.toString("dd.MM.yyyy"))
                   .arg( model_->viittausLasku().erapvm.toString("dd.MM.yyyy"));

        painter->setFont( QFont("Sans", 10));
        QRectF ltRect = painter->boundingRect(QRect(0,0,painter->window().width(), painter->window().height()), Qt::TextWordWrap, maksutieto );
        painter->drawText(ltRect, Qt::TextWordWrap, maksutieto );
        painter->translate( 0, ltRect.height() + painter->fontMetrics().height());  // Vähän väliä

        LaskuModel *alkuperainenLasku = LaskuModel::haeLasku( model_->viittausLasku().eraId );
        erittely( alkuperainenLasku, printer, painter, marginaali);
    }

    painter->resetTransform();

    return true;
}

QByteArray LaskunTulostaja::pdf(bool kaytaIkkunakuorta)
{
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);
    QPainter painter(&writer);

    writer.setCreator(QString("%1 %2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ));
    writer.setTitle( tr("Lasku %1").arg(model_->laskunro()));
    tulosta(&writer, &painter,kaytaIkkunakuorta);
    painter.end();

    buffer.close();

    return array;
}

QString LaskunTulostaja::html()
{
    QString txt = "<html><body><table width=100%>\n";


    QString osoite = model_->osoite();
    osoite.replace("\n","<br/>");

    QString omaosoite = kp()->asetukset()->asetus("Osoite");
    omaosoite.replace("\n","<br>");

    QString otsikko = t("laskuotsikko");
    if( model_->tyyppi() == LaskuModel::HYVITYSLASKU)
        otsikko = tr("Hyvityslasku laskulle %1").arg( model_->viittausLasku().viite);
    else if( model_->tyyppi() == LaskuModel::MAKSUMUISTUTUS)
        otsikko = tr("MAKSUMUISTUTUS");
    else if(model_->kirjausperuste() == LaskuModel::KATEISLASKU)
        otsikko = tr("Kuitti");

    if( kp()->asetukset()->onko("Harjoitus") && !kp()->asetukset()->onko("Demo") )
    {
        otsikko = QString("<p style='text-align:right;'><span style='color: green;'>HARJOITUS</span></p>") + otsikko;
    }

    txt.append(tr("<tr><td width=50% style=\"border-bottom: 1px solid black;\">%1<br>%2</td><td colspan=2 style='font-size: large; border-bottom: 1px solid black;'>%3</td></tr>\n").arg(kp()->asetukset()->asetus("Nimi")).arg(omaosoite).arg(otsikko) );
    txt.append(QString("<td rowspan=7 style=\"border-bottom: 1px solid black; border-right: 1px solid black;\">%1</td>").arg(osoite));

    if( model_->tyyppi() != LaskuModel::HYVITYSLASKU )
        txt.append(tr("<td width=25% style=\"border-bottom: 1px solid black;\">Laskun päivämäärä</td><td width=25% style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg( kp()->paivamaara().toString("dd.MM.yyyy") ));
    else
        txt.append(tr("<td width=25% style=\"border-bottom: 1px solid black;\">Hyvityksen päivämäärä</td><td width=25% style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg( kp()->paivamaara().toString("dd.MM.yyyy") ));

    if( model_->kirjausperuste() == LaskuModel::KATEISLASKU)
        txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">Laskun numero</td><td style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg(model_->viitenumero() ));
    else if( model_->tyyppi() == LaskuModel::HYVITYSLASKU)
        txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">Hyvityslaskun numero</td><td style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg(model_->viitenumero() ));
    else
        txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">Viitenumero</td><td style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg(model_->viitenumero() ));

    // Käteislaskulla tai hyvityslaskulla ei eräpäivää
    if( model_->kirjausperuste() != LaskuModel::KATEISLASKU && model_->tyyppi() != LaskuModel::HYVITYSLASKU)
        txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">Eräpäivä</td><td style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg(model_->erapaiva().toString("dd.MM.yyyy")));

    txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">Summa</td><td style=\"border-bottom: 1px solid black;\">%L1 €</td>\n").arg( (model_->laskunSumma() / 100.0) ,0,'f',2));

    if( model_->kirjausperuste() != LaskuModel::KATEISLASKU && model_->tyyppi() != LaskuModel::HYVITYSLASKU)
    {
        txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">IBAN</td><td style=\"border-bottom: 1px solid black;\">%1</td></tr>").arg( iban) );
    }

    if( !model_->ytunnus().isEmpty())
        txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">Asiakkaan Y-tunnus</td><td style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg( model_->ytunnus() ));
    if( !model_->asiakkaanViite().isEmpty())
        txt.append(tr("<tr><td style=\"border-bottom: 1px solid black;\">Asiakkaan Y-tunnus</td><td style=\"border-bottom: 1px solid black;\">%1</td></tr>\n").arg( model_->asiakkaanViite() ));



    QString lisatieto = model_->lisatieto();
    lisatieto.replace("\n","<br>");

    txt.append("</table><p>" + lisatieto + "</p><table width=100% style=\"margin-top: 2em; margin-bottom: 1em\">");

    bool alv = kp()->asetukset()->onko("AlvVelvollinen");

    if(alv)
        txt.append(tr("<tr><th style=\"border-bottom: 1px solid black;\">Nimike</th><th style=\"border-bottom: 1px solid black;\">Määrä</th><th style=\"border-bottom: 1px solid black;\">"
                      "á netto</th><th style=\"border-bottom: 1px solid black;\">Alv %</th><th style=\"border-bottom: 1px solid black;\">Alv</th><th style=\"border-bottom: 1px solid black;\">Yhteensä</th></tr>\n"));
    else
        txt.append(tr("<tr><th style=\"border-bottom: 1px solid black;\">Nimike</th><th style=\"border-bottom: 1px solid black;\">Määrä</th><th style=\"border-bottom: 1px solid black;\">á netto</th><th style=\"border-bottom: 1px solid black;\">Yhteensä</th></tr>\n"));


    qreal kokoNetto = 0.0;
    qreal kokoVero = 0.0;

    QMap<int,AlvErittelyEra> alvit;

    for(int i=0; i < model_->rowCount(QModelIndex());i++)
    {
        int yhtsnt = model_->data( model_->index(i, LaskuModel::BRUTTOSUMMA), Qt::EditRole ).toInt();
        if( !yhtsnt)
            continue;   // Ohitetaan tyhjät rivit

        QString nimike = model_->data( model_->index(i, LaskuModel::NIMIKE), Qt::DisplayRole ).toString();
        QString maara = model_->data( model_->index(i, LaskuModel::MAARA), Qt::DisplayRole ).toString();
        QString yksikko = model_->data( model_->index(i, LaskuModel::YKSIKKO), Qt::DisplayRole ).toString();
        QString ahinta = model_->data( model_->index(i, LaskuModel::AHINTA), Qt::DisplayRole ).toString();
        QString vero = model_->data( model_->index(i, LaskuModel::ALV), Qt::DisplayRole ).toString();
        QString yht = model_->data( model_->index(i, LaskuModel::BRUTTOSUMMA), Qt::DisplayRole ).toString();

        double verosnt = model_->data( model_->index(i, LaskuModel::NIMIKE), LaskuModel::VeroRooli ).toDouble();
        double nettosnt = model_->data( model_->index(i, LaskuModel::NIMIKE), LaskuModel::NettoRooli ).toDouble();
        int veroprossa = model_->data( model_->index(i, LaskuModel::NIMIKE), LaskuModel::AlvProsenttiRooli ).toInt();

        kokoNetto += nettosnt;

        if( alv )
        {
            txt.append(QString("<tr><td>%1</td><td style='text-align:right;'>%2 %3</td><td style='text-align:right;'>%4</td><td style='text-align:right;'>%5</td><td style='text-align:right;'>%L6 €</td><td style='text-align:right;'>%7</td></tr>\n")
                       .arg(nimike).arg(maara).arg(yksikko).arg(ahinta).arg(vero).arg(verosnt / 100.0,0,'f',2).arg(yht) );

            // Lisätään alv-erittelyä varten summataulukkoihin
            if( !alvit.contains(veroprossa))
                alvit.insert(veroprossa, AlvErittelyEra() );

            alvit[veroprossa].netto += nettosnt;
            alvit[veroprossa].vero += verosnt;
            kokoVero += verosnt;
        }
        else
        {
            txt.append(QString("<tr><td>%1</td><td style='text-align:right;'>%2 %3</td><td style='text-align:right;'>%4</td><td style='text-align:right;'>%5</td></tr>\n")
                       .arg(nimike).arg(maara).arg(yksikko).arg(ahinta).arg(yht) );            
        }
    }
    txt.append("</table><p>");
    if( alv && model_->tyyppi() != LaskuModel::MAKSUMUISTUTUS)
    {
        txt.append(tr("<table width=50% style=\"margin-left: auto;\"><tr><th style=\"border-bottom: 1px solid black;\">Alv%</th><th style=\"border-bottom: 1px solid black;\">Veroton</th><th style=\"border-bottom: 1px solid black;\">Vero</th><th style=\"border-bottom: 1px solid black;\">Yhteensä</th></tr>"));
        QMapIterator<int,AlvErittelyEra> iter(alvit);
        while(iter.hasNext())
        {
            iter.next();
            txt.append( tr("<tr><td>%1 %</td><td style='text-align:right;'>%L2 €</td><td style='text-align:right;'>%L3 €</td><td  style='text-align:right;'>%L4 €</td><tr>\n")
                        .arg( iter.key())
                        .arg( ( iter.value().netto / 100.0) ,0,'f',2)
                        .arg( ( iter.value().vero / 100.0) ,0,'f',2)
                        .arg( (iter.value().brutto() / 100.0) ,0,'f',2) ) ;
        }
        txt.append( tr("<tr><td><b>Yhteensä</b> </td><td td style='text-align:right;'><b>%L1 €</b></td><td style='text-align:right;'><b>%L2 €</b></td><td style='text-align:right;'><b>%L3 €</b></td><tr>\n")
                    .arg( ( kokoNetto / 100.0) ,0,'f',2)
                    .arg( ( kokoVero / 100.0) ,0,'f',2)
                    .arg( ( model_->laskunSumma() / 100.0) ,0,'f',2) ) ;
        txt.append("</table>");

    }

    if( virtuaaliviivakoodi().length() > 50)
        txt.append(tr("<hr><p>Virtuaaliviivakoodi <b>%1</b></p><hr>").arg( virtuaaliviivakoodi()) );

    txt.append("<table>");
    if( kp()->asetukset()->asetus("Ytunnus").length())
        txt.append(tr("<tr><td>Y-tunnus </td><td>%1</td></tr>\n").arg(kp()->asetukset()->asetus("Ytunnus")));
    if( kp()->asetukset()->asetus("Puhelin").length())
        txt.append(tr("<tr><td>Puhelin </td><td>%2</td></tr> \n").arg(kp()->asetukset()->asetus("Puhelin")));
    if( kp()->asetukset()->asetus("Sahkoposti").length())
        txt.append(tr("<tr><td>Sähköposti </td><td>%2</td></tr> \n").arg(kp()->asetukset()->asetus("Sahkoposti")));

    txt.append("</table></body></html>\n");

    return txt;
}

QString LaskunTulostaja::virtuaaliviivakoodi() const
{
    if( model_->laskunSumma() > 99999999 )  // Ylisuuri laskunsumma
        return QString();

    QString koodi = QString("4 %1 %2 000 %3 %4")
            .arg( iban.mid(2,16) )  // Tilinumeron numeerinen osuus
            .arg( model_->laskunSumma(), 8, 10, QChar('0') )  // Rahamäärä
            .arg( model_->viitenumero().remove(QChar(' ')), 20, QChar('0'))
            .arg( model_->erapaiva().toString("yyMMdd"));

    return koodi.remove(QChar(' '));
}

QString LaskunTulostaja::valeilla(const QString &teksti)
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

void LaskunTulostaja::asetaKieli(const QString &kieli)
{
    kieli_ = kieli;
}

QString LaskunTulostaja::t(const QString &avain) const
{
    // Ensisijaisesti palautetaan nykyisellä kielellä
    if( tekstit_.contains( kieli_ + avain ))
        return  tekstit_.value( kieli_ + avain );
    // Ellei ole käännöstä nykyiselle kielelle, palautetaan oletusteksti (suomea)
    return tekstit_.value( avain, avain );
}

QString LaskunTulostaja::veroteksti(int verokoodi) const
{
    switch (verokoodi) {
    case AlvKoodi::EIALV:
        return t("veroton");
    case AlvKoodi::YHTEISOMYYNTI_TAVARAT:
        return t("yhtmy");
    case AlvKoodi::YHTEISOMYYNTI_PALVELUT:
        return t("palmy");
    case AlvKoodi::RAKENNUSPALVELU_MYYNTI:
        return t("rakmy");
    case LaskuModel::Kaytetyt:
        return t("vmkt");
    case LaskuModel::Taide:
        return t("vmte");
    case LaskuModel::KerailyAntiikki:
        return t("vmka");
    }
    return "!" + QString::number(verokoodi);
}

void LaskunTulostaja::ylaruudukko(QPagedPaintDevice *printer, QPainter *painter, bool kaytaIkkunakuorta)
{
    const int TEKSTIPT = 10;
    const int OTSAKEPT = 7;

    double mm = printer->width() * 1.00 / printer->widthMM();

    // Lasketaan rivinkorkeus. Tehdään painterin kautta, jotta toimii myös pdf-writerillä
    painter->setFont( QFont("Sans",OTSAKEPT) );
    double rk = painter->fontMetrics().height();
    painter->setFont(QFont("Sans",TEKSTIPT));
    rk += painter->fontMetrics().height();
    rk += 2 * mm;

    double leveys = painter->window().width();

    // Kuoren ikkuna
    QRectF ikkuna;
    double keskiviiva = leveys / 2;

    if( kp()->asetukset()->onko("LaskuIkkuna") && kaytaIkkunakuorta)
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
    if( kp()->asetukset()->luku("LaskuIkkunaKorkeus", 35) > 55 )
    {
        lahettajaAlue = QRectF( ikkuna.x(), ikkuna.y(), ikkuna.width(), 30 * mm);
        ikkuna = QRectF( ikkuna.x(), ikkuna.y() + 30 * mm, ikkuna.width(), ikkuna.height() - 30 * mm);
    }


    // Lähettäjätiedot

    double vasen = 0.0;
    if( !kp()->logo().isNull() )
    {
        double logosuhde = (1.0 * kp()->logo().width() ) / kp()->logo().height();
        double skaala = logosuhde < 5.00 ? logosuhde : 5.00;    // Logon sallittu suhde enintään 5:1

        painter->drawImage( QRectF( lahettajaAlue.x()+mm, lahettajaAlue.y()+mm, rk*2*skaala, rk*2 ),  kp()->logo()  );
        vasen += rk * 2.2 * skaala;

    }
    painter->setFont(QFont("Sans",14));
    double pv = painter->fontMetrics().height();
    QString nimi = kp()->asetukset()->onko("LogossaNimi") ? QString() : kp()->asetus("Nimi");   // Jos nimi logossa, sitä ei toisteta
    QRectF lahettajaRect = painter->boundingRect( QRectF( lahettajaAlue.x()+vasen, lahettajaAlue.y(),
                                                       lahettajaAlue.width()-vasen, 20 * mm), Qt::TextWordWrap, nimi );
    painter->drawText(QRectF( lahettajaRect), Qt::AlignLeft | Qt::TextWordWrap, nimi);

    painter->setFont(QFont("Sans",9));
    QRectF lahettajaosoiteRect = painter->boundingRect( QRectF( lahettajaAlue.x()+vasen, lahettajaAlue.y() + lahettajaRect.height(),
                                                       lahettajaAlue.width()-vasen, 20 * mm), Qt::TextWordWrap, kp()->asetus("Osoite") );
    painter->drawText(lahettajaosoiteRect, Qt::AlignLeft, kp()->asetus("Osoite") );

    // Tulostetaan saajan osoite ikkunaan
    painter->setFont(QFont("Sans", TEKSTIPT));
    painter->drawText(ikkuna, Qt::TextWordWrap, model_->osoite());

    pv += rk ;     // pv = perusviiva

    painter->setPen( QPen( QBrush(Qt::black), 0.13));
    painter->drawLine( QLineF(keskiviiva, pv, keskiviiva,  model_->asiakkaanViite().isEmpty() ? pv+2 : pv + 3  ));
    for(int i=-1; model_->asiakkaanViite().isEmpty() ? i < 3 : i < 2; i++)
        painter->drawLine(QLineF(keskiviiva, pv + i * rk, leveys, pv + i * rk));

    painter->drawLine( QLineF(keskiviiva, pv-rk, leveys, pv-rk ));
    if( !model_->ytunnus().isEmpty())
        painter->drawLine( QLineF(puoliviiva, pv, puoliviiva, pv+rk ));

    painter->drawLine(QLineF(puoliviiva, pv-rk, puoliviiva, pv));
    painter->drawLine(QLineF(puoliviiva, pv+rk, puoliviiva, pv+rk*2));

    painter->setFont( QFont("Sans",OTSAKEPT) );

    if( model_->asiakkaanViite().isEmpty())
    {
        painter->drawLine(QLineF(keskiviiva, pv-rk, keskiviiva, pv + 2 * rk));
    }
    else
    {
        painter->drawLine(QLineF(keskiviiva, pv + 3 * rk, leveys, pv + 3 * rk));
        painter->drawLine( QLineF(keskiviiva, pv-rk, keskiviiva, pv+rk*3));
        painter->drawText(QRectF( keskiviiva + mm, pv + rk * 4 + mm, leveys / 4, rk ), Qt::AlignTop, t("asviite"));
    }


    painter->drawText(QRectF( keskiviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("pvm"));

    if( model_->tyyppi() == LaskuModel::HYVITYSLASKU)
        painter->drawText(QRectF( puoliviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("hyvnro"));
    else
        painter->drawText(QRectF( puoliviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("lnro"));

    if( model_->tyyppi() == LaskuModel::HYVITYSLASKU )
        painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk ), Qt::AlignTop, t("hyvpvm"));
    else
        painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk ), Qt::AlignTop, t("toimpvm"));

    if( !model_->ytunnus().isEmpty())
        painter->drawText(QRectF( puoliviiva + mm, pv + mm, leveys / 4, rk), Qt::AlignTop, t("asytunnus"));

    painter->drawText(QRectF( keskiviiva + mm, pv + rk + mm, leveys / 4, rk ), Qt::AlignTop, t("huomaika"));
    painter->drawText(QRectF( puoliviiva + mm, pv + rk + mm, leveys / 4, rk ), Qt::AlignTop, t("viivkorko"));

    painter->setFont(QFont("Sans", TEKSTIPT));

    painter->drawText(QRectF( keskiviiva + mm, pv - rk, leveys / 4, rk-mm ), Qt::AlignBottom, kp()->paivamaara().toString("dd.MM.yyyy") );
    painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk-mm ), Qt::AlignBottom,  model_->toimituspaiva().toString("dd.MM.yyyy") );
    painter->drawText(QRectF( puoliviiva + mm, pv - rk, leveys / 2, rk-mm ), Qt::AlignBottom, model_->viitenumero() );

    if( !model_->ytunnus().isEmpty())
        painter->drawText(QRectF( puoliviiva + mm, pv + mm, leveys / 4, rk-mm ), Qt::AlignBottom,  model_->ytunnus() );


    if( model_->kirjausperuste() == LaskuModel::KATEISLASKU)
    {
        painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, leveys / 4, rk-mm ), Qt::AlignBottom,  t("kateinen") );
    }
    else if( model_->tyyppi() == LaskuModel::HYVITYSLASKU)
    {
        painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, leveys - keskiviiva, rk-mm ), Qt::AlignBottom,  QString("%1 %2").arg(t("hlasku"))
                          .arg( model_->viittausLasku().viite ));
    }
    else
    {
        if( model_->tyyppi() == LaskuModel::MAKSUMUISTUTUS)
        {
            painter->setFont( QFont("Sans", TEKSTIPT+2,QFont::Black));
            painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, (leveys - keskiviiva)/2, rk-mm ), Qt::AlignBottom,  t("maksumuistutus") );
            painter->setFont(QFont("Sans", TEKSTIPT));
        }
        else
            painter->drawText(QRectF( keskiviiva + mm, pv - rk * 2, (leveys -keskiviiva)/2, rk-mm ), Qt::AlignBottom,  t("laskuotsikko") );

        if( model_->laskunSumma() > 0.0 )  // Näytetään eräpäivä vain jos on maksettavaa
        {
            painter->drawText(QRectF( puoliviiva + mm, pv + rk, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  kp()->asetus("LaskuViivastyskorko") );
        }
    }
    painter->drawText(QRectF( keskiviiva + mm, pv + rk, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  kp()->asetus("LaskuHuomautusaika") );

    painter->drawText(QRectF( keskiviiva + mm, pv + rk * 2, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  model_->asiakkaanViite() );

    // Kirjoittamista jatkettaan ruudukon jälkeen - taikka ikkunan, jos se on isompi
    qreal ruutukorkeus = model_->asiakkaanViite().isEmpty() ? pv + rk * 2 : pv + rk *3;
    qreal ikkunakorkeus = ikkuna.y() + ikkuna.height() + 5 * mm;

    painter->setFont( QFont("Sans", 10));
    QString lisatieto = model_->lisatieto();

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


qreal LaskunTulostaja::alatunniste(QPagedPaintDevice *printer, QPainter *painter)
{
    painter->save();
    painter->setFont( QFont("Sans",10));
    qreal rk = painter->fontMetrics().height();
    painter->translate(0, -4.5 * rk);

    qreal leveys = painter->window().width();
    double mm = printer->width() * 1.00 / printer->widthMM();

    painter->setPen( QPen(QBrush(Qt::black), mm * 0.2));
    painter->drawRect(QRectF(leveys * 4 / 5, 0, leveys / 5, rk * 2));

    painter->setFont( QFont("Sans",8));
    painter->drawText(QRectF(leveys * 4 / 5 + mm, 0 + mm, leveys / 5, rk), t("Yhteensa"));
    painter->setFont( QFont("Sans", 11,QFont::Bold));
    painter->drawText(QRectF(leveys * 4 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight,QString("%L1 €").arg( (model_->laskunSumma() / 100.0) ,0,'f',2) );

    if( model_->laskunSumma()>0 )
    {
        painter->drawRect(QRectF(leveys * 3 / 5, 0, leveys / 5, rk * 2));
        painter->setFont( QFont("Sans",8));
        painter->drawText(QRectF(leveys * 3 / 5 + mm, 0 + mm, leveys / 5, rk), t("erapvm"));
        painter->setFont( QFont("Sans", 11));


        if(model_->kirjausperuste() == LaskuModel::KATEISLASKU )
        {
            painter->drawText(QRectF(leveys * 3 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight, t("maksettu"));
        }
        else
        {
            painter->drawText(QRectF(leveys * 3 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight, model_->erapaiva().toString("dd.MM.yyyy"));

            painter->drawRect(QRectF(leveys * 2 / 5, 0, leveys / 5, rk * 2));
            painter->drawRect(QRectF(0, 0, leveys * 2 / 5, rk * 2));

            painter->setFont( QFont("Sans",8));
            painter->drawText(QRectF(leveys * 2 / 5 + mm, 0 + mm, leveys / 5, rk), t("viitenro"));
            painter->drawText(QRectF(mm, 0 + mm, leveys * 2 / 5, rk), t("iban"));

            painter->setFont( QFont("Sans", 11));
            painter->drawText(QRectF(leveys * 2 / 5, 0, leveys / 5-mm, rk * 2), Qt::AlignBottom | Qt::AlignRight, valeilla(model_->viitenumero()) );
            painter->drawText(QRectF(0, 0, leveys * 2 / 5 -mm, rk * 2), Qt::AlignBottom | Qt::AlignRight, valeilla(iban) );
        }
    }

    painter->translate(0, 2.1 * rk );
    painter->setFont(QFont("Sans",8));
    painter->drawText(QRectF(0,0,leveys/5*2,rk), Qt::AlignLeft, kp()->asetukset()->asetus("Nimi")  );
    painter->drawText(QRectF(0, painter->fontMetrics().height(),leveys/5*2,rk), Qt::AlignLeft, kp()->asetukset()->asetus("Sahkoposti")  );

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
        painter->drawText(QRectF(leveys*4 / 5,rk,leveys/3,painter->fontMetrics().height()*2), Qt::AlignLeft | Qt::TextWordWrap , t("eialv"));


    // Viivakoodi
    if( !kp()->asetukset()->onko("LaskuEiViivakoodi") && kp()->asetukset()->onko("LaskuEiTilisiirto") && model_->laskunSumma() > 0 && model_->kirjausperuste() != LaskuModel::KATEISLASKU)
    {
        QFont koodifontti( "code128_XL", 36);
        koodifontti.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
        painter->setFont( koodifontti);
        QString koodi( code128() );
        painter->drawText( QRectF( mm*20, -2.2*rk-mm*15, mm*100, mm*10), Qt::AlignCenter, koodi  );

        painter->restore();
        return  4.7 * rk + mm * 16;
    }

    painter->restore();
    return 4.5 * rk;
}

void LaskunTulostaja::erittely(LaskuModel *model, QPagedPaintDevice *printer, QPainter *painter, qreal marginaali)
{

    ErittelyRuudukko ruudukko(model, this);
    ruudukko.tulostaErittely(printer, painter, marginaali);

    // ALV-erittelyn tulostus
    qreal rk = painter->fontMetrics().height();
    qreal leveys = painter->window().width();
    double mm = printer->width() * 1.00 / printer->widthMM();

    QList<AlvErittelyRivi> alvLista = model->alverittely();


    // Tarvittaessa vaihdetaan sivua
    if( painter->transform().dy() + rk * (alvLista.count()+2.5)  >  painter->window().height() - marginaali )
    {
        printer->newPage();
        painter->resetTransform();
        painter->drawText( QRectF(0,0,leveys/2,rk), Qt::AlignLeft, kp()->asetukset()->asetus("Nimi"));
        painter->drawText( QRectF(leveys/2,0,leveys/2, rk), Qt::AlignRight, kp()->paivamaara().toString("dd.MM.yyyy"));
        painter->translate(0, rk*2);
    }

    bool alv = kp()->asetukset()->onko("AlvVelvollinen");
    if( model->tyyppi() != LaskuModel::MAKSUMUISTUTUS && !alvLista.isEmpty())
    {
        painter->translate( 0, rk * 0.5);

        if( alvLista.first().vero() > 1e-7)
        {
            painter->setFont(QFont("Sans",8));
            painter->drawText(QRectF( leveys * 5 / 8, 0, leveys / 8, rk), Qt::AlignRight, t("veroton"));
            painter->drawText(QRectF( leveys * 6 / 8, 0, leveys / 8, rk), Qt::AlignRight, t("vero"));
            painter->drawText(QRectF( leveys * 7 / 8, 0, leveys / 8, rk), Qt::AlignRight, t("verollinen"));
            painter->translate(0, painter->fontMetrics().height());
            painter->setFont(QFont("Sans",10));
        }

        painter->drawLine(QLineF(3*leveys / 8.0, 0, leveys, 0));

        for( AlvErittelyRivi alvRivi : alvLista )
        {
            if( alvRivi.alvKoodi() != AlvKoodi::MYYNNIT_NETTO && alvRivi.alvKoodi() != AlvKoodi::ALV0)
            {
                painter->drawText(QRectF(3 * leveys / 8,0, leveys/2,rk), Qt::AlignLeft, veroteksti(alvRivi.alvKoodi())  );
            }
            else
            {
                painter->drawText(QRectF(3 *leveys / 8,0,leveys/4,rk), Qt::AlignLeft, QString("%1 %2 %").arg(t("alv")).arg( alvRivi.alvProsentti() ) );
                painter->drawText(QRectF(5 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( ( alvRivi.netto() / 100.0) ,0,'f',2)  );
                painter->drawText(QRectF(6 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( ( (alvRivi.brutto() - alvRivi.netto()) / 100.0) ,0,'f',2) );
            }
            painter->drawText(QRectF(7*leveys/8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( ( alvRivi.brutto() / 100.0) ,0,'f',2) );
            painter->translate(0, rk);

        }
    }

    painter->translate(0, rk * 0.25);

    qreal yhtviivaAlkaa = alv == true ? 3 * leveys / 8 : 10 * leveys / 16.0; // ilman alviä lyhyempi yhteensä-viiva

    painter->drawLine(QLineF(yhtviivaAlkaa, -0.26 * mm , leveys, -0.26 * mm));
    painter->drawLine(QLineF(yhtviivaAlkaa, 0, leveys, 0));
    painter->drawText(QRectF(yhtviivaAlkaa, 0,leveys/8,rk), Qt::AlignLeft, t("Yhteensa")  );

    if( alv && alvLista.first().vero() > 1e-7)
    {
        painter->drawText(QRectF(5 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( ( model->nettoSumma()  / 100.0) ,0,'f',2)  );
        painter->drawText(QRectF(6 *leveys / 8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( ( (model->laskunSumma() - model->nettoSumma() ) / 100.0) ,0,'f',2) );
    }
    painter->drawText(QRectF(7*leveys/8,0,leveys/8,rk), Qt::AlignRight, QString("%L1 €").arg( ( model->laskunSumma() / 100.0) ,0,'f',2) );
    painter->translate(0, 2*rk);


}

void LaskunTulostaja::erittelyOtsikko(QPagedPaintDevice *printer, QPainter *painter, bool alv)
{    
    painter->setFont( QFont("Sans",8));
    qreal rk = painter->fontMetrics().height();
    qreal leveys = painter->window().width();
    double mm = printer->width() * 1.00 / printer->widthMM();

    painter->drawText(QRectF(0,0,leveys/2,rk), Qt::AlignLeft, tr("Nimike"));

    if( alv )
    {
        painter->drawText(QRectF(5 *leveys / 16,0,leveys/8,rk), Qt::AlignHCenter, t("kpl"));
        painter->drawText(QRectF(7 *leveys / 16,0,leveys/8,rk), Qt::AlignHCenter, t("anetto"));
        painter->drawText(QRectF(9 *leveys / 16,0,leveys/8,rk), Qt::AlignHCenter, t("netto"));
        painter->drawText(QRectF(11 *leveys / 16,0,leveys/16,rk), Qt::AlignHCenter, t("alv"));
        painter->drawText(QRectF(12 *leveys / 16,0,leveys/8,rk), Qt::AlignHCenter, t("vero"));
    }
    else
    {
        painter->drawText(QRectF(5 *leveys / 8,0,leveys/8,rk), Qt::AlignHCenter, t("kpl"));
        painter->drawText(QRectF(6 *leveys / 8,0,leveys/8,rk), Qt::AlignHCenter, t("ahinta"));
    }
    painter->drawText(QRectF(7 *leveys / 8,0,leveys/8,rk), Qt::AlignHCenter, t("yhteensa"));
    painter->setPen( QPen(QBrush(Qt::black), mm * 0.13));
    painter->drawLine( QLineF( 0, rk, leveys, rk));
    painter->translate(0, rk * 1.1);
    painter->setFont( QFont("Sans",10));

}

void LaskunTulostaja::tilisiirto(QPagedPaintDevice *printer, QPainter *painter)
{
    painter->setFont(QFont("Sans", 7));
    double mm = printer->width() * 1.00 / printer->widthMM();

    // QR-koodi
    if( !kp()->asetukset()->onko("LaskuEiQR"))
    {
        QByteArray qrTieto = qrSvg();
        if( !qrTieto.isEmpty())
        {
            QSvgRenderer qrr( qrTieto );
            qrr.render( painter, QRectF( ( printer->widthMM() - 35 ) *mm, 5 * mm, 30 * mm, 30 * mm  ) );
        }
    }

    painter->drawText( QRectF(0,0,mm*19,mm*16.9), Qt::AlignRight | Qt::AlignHCenter, t("bst"));
    painter->drawText( QRectF(0, mm*18, mm*19, mm*14.8), Qt::AlignRight | Qt::AlignHCenter, t("bsa"));
    painter->drawText( QRectF(0, mm*32.7, mm*19, mm*20), Qt::AlignRight | Qt::AlignTop, t("bmo"));
    painter->drawText( QRectF(0, mm*51.3, mm*19, mm*10), Qt::AlignRight | Qt::AlignBottom , t("bak"));
    painter->drawText( QRectF(0, mm*62.3, mm*19, mm*8.5), Qt::AlignRight | Qt::AlignHCenter, t("btl"));
    painter->drawText( QRectF(mm * 22, 0, mm*20, mm*10), Qt::AlignLeft, t("iban"));

    painter->drawText( QRectF(mm*112.4, mm*53.8, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, t("bvn"));
    painter->drawText( QRectF(mm*112.4, mm*62.3, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, t("bep"));
    painter->drawText( QRectF(mm*159, mm*62.3, mm*19, mm*8.5), Qt::AlignLeft, t("eur"));


    painter->setFont(QFont("Sans",6));
    painter->drawText( QRectF( mm * 140, mm * 72, mm * 60, mm * 20), Qt::AlignLeft | Qt::TextWordWrap, t("behto") );
    painter->setPen( QPen( QBrush(Qt::black), mm * 0.5));
    painter->drawLine(QLineF(mm*111.4,0,mm*111.4,mm*69.8));
    painter->drawLine(QLineF(0, mm*16.9, mm*111.4, mm*16.9));
    painter->drawLine(QLineF(0, mm*31.7, mm*111.4, mm*31.7));
    painter->drawLine(QLineF(mm*20, 0, mm*20, mm*31.7));
    painter->drawLine(QLineF(0, mm*61.3, mm*200, mm*61.3));
    painter->drawLine(QLineF(0, mm*69.8, mm*200, mm*69.8));
    painter->drawLine(QLineF(mm*111.4, mm*52.8, mm*200, mm*52.8));
    painter->drawLine(QLineF(mm*131.4, mm*52.8, mm*131.4, mm*69.8));
    painter->drawLine(QLineF(mm*158, mm*61.3, mm*158, mm*69.8));
    painter->drawLine(QLineF(mm*20, mm*61.3, mm*20, mm*69.8));

    painter->setPen( QPen(QBrush(Qt::black), mm * 0.13));
    painter->drawLine( QLineF( mm*22, mm*57.1, mm*108, mm*57.1));

    painter->setPen( QPen(QBrush(Qt::black), mm * 0.13, Qt::DashLine));
    painter->drawLine( QLineF( 0, -1 * mm, painter->window().width(), -1 * mm));

    painter->setFont(QFont("Sans", 10));

    painter->drawText(QRectF( mm*22, mm * 33, mm * 90, mm * 25), Qt::TextWordWrap, model_->osoite());

    painter->drawText( QRectF(mm*133.4, mm*53.8, mm*60, mm*7.5), Qt::AlignLeft | Qt::AlignBottom, model_->viitenumero() );

    painter->drawText( QRectF(mm*133.4, mm*62.3, mm*30, mm*7.5), Qt::AlignLeft | Qt::AlignBottom, model_->erapaiva().toString("dd.MM.yyyy") );
    painter->drawText( QRectF(mm*165, mm*62.3, mm*30, mm*7.5), Qt::AlignRight | Qt::AlignBottom, QString("%L1").arg( (model_->laskunSumma() / 100.0) ,0,'f',2) );

    painter->drawText( QRectF(mm*22, mm*17, mm*90, mm*13), Qt::AlignTop | Qt::TextWordWrap, kp()->asetus("Nimi") + "\n" + kp()->asetus("Osoite")  );
    painter->drawText( QRectF(mm*22, 0, mm*90, mm*17), Qt::AlignVCenter ,  valeilla( iban )  );


    painter->save();
    painter->setFont(QFont("Sans", 7));
    painter->translate(mm * 2, mm* 60);
    painter->rotate(-90.0);
    painter->drawText(0,0,t("btilis"));
    painter->restore();

    // Viivakoodi
    if( !kp()->asetukset()->onko("LaskuEiViivakoodi"))
    {
        painter->save();

        QFont koodifontti( "code128_XL", 36);
        koodifontti.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
        painter->setFont( koodifontti);
        QString koodi( code128() );
        painter->drawText( QRectF( mm*20, mm*72, mm*100, mm*13), Qt::AlignCenter, koodi  );

        painter->restore();
    }
}

QString LaskunTulostaja::code128() const
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

QChar LaskunTulostaja::code128c(int koodattava) const
{
    if( koodattava < 95)
        return QChar( 32 + koodattava);
    else
        return QChar( 105 + koodattava);
}

QByteArray LaskunTulostaja::qrSvg() const
{
    // Esitettävä tieto
    QString data("BCD\n001\n1\nSCT\n");

    QString bic = LaskutModel::bicIbanilla(iban);
    if( bic.isEmpty())
        return QByteArray();
    data.append(bic + "\n");
    data.append(kp()->asetukset()->asetus("Nimi") + "\n");
    data.append(iban + "\n");
    data.append( QString("EUR%1.%2\n\n").arg( model_->laskunSumma() / 100 ).arg( model_->laskunSumma() % 100, 2, 10, QChar('0') ));
    data.append(model_->viitenumero().remove(QChar(' ')) + "\n\n");
    data.append( QString("ReqdExctnDt/%1").arg( model_->erapaiva().toString(Qt::ISODate) ));

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText( data.toUtf8().data() , qrcodegen::QrCode::Ecc::QUARTILE);
    return QByteArray::fromStdString( qr.toSvgString(1) );
}

