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

#include "validator/ibanvalidator.h"

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
    MyyntiLaskunTulostaja *tulostaja = new MyyntiLaskunTulostaja( lasku );
    double mm = printer->width() * 1.00 / printer->widthMM();
    qreal marginaali = 0.0;


    bool ikkunakuori = kuoreen & kp()->asetukset()->onko("IkkunaKuori");

    if( tulostaja->laskunSumma_ > 0.0 )
    {
        painter->translate( 0, painter->window().height() - mm * 95 );
    //    marginaali += alatunniste(printer, painter) + mm * 95;
        tulostaja->tilisiirto(printer, painter);
        painter->resetTransform();
    }


    tulostaja->ylaruudukko(printer, painter, ikkunakuori);


    tulostaja->deleteLater();
    return true;
}

QByteArray MyyntiLaskunTulostaja::pdf(const QVariantMap &lasku, bool ikkunakuoreen)
{
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open(QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);
    QPainter painter(&writer);

    writer.setCreator(QString("%1 %2").arg( qApp->applicationName() ).arg( qApp->applicationVersion() ));
    writer.setTitle( tr("Lasku %1").arg( lasku.value("lasku").toMap().value("numero").toInt()) );
    tulosta(lasku, &writer, &painter, ikkunakuoreen);
    painter.end();

    buffer.close();

    return array;
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

MyyntiLaskunTulostaja::MyyntiLaskunTulostaja(const QVariantMap& map, QObject *parent) :
    QObject(parent), map_(map), rivit_( this, map.value("rivit").toList() ),
    laskunSumma_( rivit_.yhteensa() )
{
    QString kieli = map_.value("lasku").toMap().value("kieli").toString();

    // Ladataan tekstit
    QFile tiedosto(":/lasku/laskutekstit.txt");
    tiedosto.open(QIODevice::ReadOnly);

    QTextStream in(&tiedosto);
    in.setCodec("utf-8");

    while( !in.atEnd())
        tekstiRivinLisays(in.readLine(), kieli);
    for( QString rivi : kp()->asetukset()->lista("LaskuTekstit"))
        tekstiRivinLisays( rivi, kieli);

    iban_ = kp()->tilit()->tiliNumerolla( kp()->asetukset()->luku("LaskuTili")).json()->str("IBAN");
}

void MyyntiLaskunTulostaja::ylaruudukko( QPagedPaintDevice *printer, QPainter *painter, bool kaytaIkkunakuorta)
{
    const int TEKSTIPT = 10;
    const int OTSAKEPT = 7;

    double mm = printer->width() * 1.00 / printer->widthMM();
    painter->setPen( QPen(QBrush(Qt::black), mm * 0.2));

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
    if( !kp()->logo().isNull() )
    {
        double logosuhde = (1.0 * kp()->logo().width() ) / kp()->logo().height();
        double skaala = logosuhde < 5.00 ? logosuhde : 5.00;    // Logon sallittu suhde enintään 5:1

        painter->drawImage( QRectF( lahettajaAlue.x()+mm, lahettajaAlue.y()+mm, rk*2*skaala, rk*2 ),  kp()->logo()  );
        vasen += rk * 2.2 * skaala;

    }
    painter->setFont(QFont("Sans",14));
    double pv = painter->fontMetrics().height();
    QString nimi = kp()->asetukset()->onko("LogossaNimi") ? QString() : ( kp()->asetukset()->asetus("LaskuAputoiminimi").isEmpty() ? kp()->asetukset()->asetus("Nimi") : kp()->asetukset()->asetus("LaskuAputoiminimi") );   // Jos nimi logossa, sitä ei toisteta
    QRectF lahettajaRect = painter->boundingRect( QRectF( lahettajaAlue.x()+vasen, lahettajaAlue.y(),
                                                       lahettajaAlue.width()-vasen, 20 * mm), Qt::TextWordWrap, nimi );
    painter->drawText(QRectF( lahettajaRect), Qt::AlignLeft | Qt::TextWordWrap, nimi);

    painter->setFont(QFont("Sans",9));
    QRectF lahettajaosoiteRect = painter->boundingRect( QRectF( lahettajaAlue.x()+vasen, lahettajaAlue.y() + lahettajaRect.height(),
                                                       lahettajaAlue.width()-vasen, 20 * mm), Qt::TextWordWrap, kp()->asetus("Osoite") );
    painter->drawText(lahettajaosoiteRect, Qt::AlignLeft, kp()->asetus("Osoite") );

    // Tulostetaan saajan osoite ikkunaan
    painter->setFont(QFont("Sans", TEKSTIPT));
    painter->drawText(ikkuna, Qt::TextWordWrap, map_.value("lasku").toMap().value("osoite").toString() );

    pv += rk ;     // pv = perusviiva

    QString asviite = map_.value("lasku").toMap().value("asviite").toString();
    painter->drawLine( QLineF(keskiviiva, pv, keskiviiva,  asviite.isEmpty() ? pv+2 : pv + 3  ));
    for(int i=-1; asviite.isEmpty() ? i < 3 : i < 2; i++)
        painter->drawLine(QLineF(keskiviiva, pv + i * rk, leveys, pv + i * rk));

    painter->drawLine( QLineF(keskiviiva, pv-rk, leveys, pv-rk ));
//    if( !model_->ytunnus().isEmpty())
//        painter->drawLine( QLineF(puoliviiva, pv, puoliviiva, pv+rk ));

    painter->drawLine(QLineF(puoliviiva, pv-rk, puoliviiva, pv));

    painter->setFont( QFont("Sans",OTSAKEPT) );

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

 /*   if( model_->tyyppi() == LaskuModel::HYVITYSLASKU)
        painter->drawText(QRectF( puoliviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("hyvnro"));
    else
        painter->drawText(QRectF( puoliviiva + mm, pv - rk + mm, leveys / 4, rk ), Qt::AlignTop, t("lnro"));

    if( model_->tyyppi() == LaskuModel::HYVITYSLASKU )
        painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk ), Qt::AlignTop, t("hyvpvm"));
    else
        painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk ), Qt::AlignTop, t("toimpvm"));

    if( !model_->ytunnus().isEmpty())
    {
        if( model_->ytunnus().at(0).isNumber())
            painter->drawText(QRectF( puoliviiva + mm, pv + mm, leveys / 4, rk), Qt::AlignTop, t("asytunnus"));
        else
            painter->drawText(QRectF( puoliviiva + mm, pv + mm, leveys / 4, rk), Qt::AlignTop, t("asalvtunnus"));
    }
*/
    painter->drawText(QRectF( keskiviiva + mm, pv + rk + mm, leveys / 4, rk ), Qt::AlignTop, t("huomaika"));

    // Viivästyskorko-laatikko vain, jos viivästyskorko määritelty ja laskulla maksettavaa

    double yhteensa = rivit_.yhteensa();
    double viivkorko = map_.value("lasku").toMap().value("viivkorko").toDouble();


    if( yhteensa > 0.0 &&  viivkorko > 1e-5 && map_.value("laskutapa") != 3 )  // TODO: Tämä vakio!!
    {
        painter->drawText(QRectF( puoliviiva + mm, pv + rk + mm, leveys / 4, rk ), Qt::AlignTop, t("viivkorko"));
        painter->drawLine(QLineF(puoliviiva, pv+rk, puoliviiva, pv+rk*2));
    }

    painter->setFont(QFont("Sans", TEKSTIPT));

    painter->drawText(QRectF( keskiviiva + mm, pv - rk, leveys / 4, rk-mm ), Qt::AlignBottom, map_.value("pvm").toDate().toString("dd.MM.yyyy") );
    painter->drawText(QRectF( keskiviiva + mm, pv + mm, leveys / 4, rk-mm ), Qt::AlignBottom, map_.value("toimituspvm").toDate().toString("dd.MM.yyyy") );
    painter->drawText(QRectF( puoliviiva + mm, pv - rk, leveys / 2, rk-mm ), Qt::AlignBottom, map_.value("lasku").toMap().value("numero").toString() );

/*    if( !model_->ytunnus().isEmpty())
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

        if( model_->laskunSumma() > 0.0 &&  model_->viivastysKorko() > 1e-5 )
        {
            painter->drawText(QRectF( puoliviiva + mm, pv + rk, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  QString("%L1 %").arg(model_->viivastysKorko(),0,'f',1) );
        }
    }
*/    painter->drawText(QRectF( keskiviiva + mm, pv + rk, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  kp()->asetus("LaskuHuomautusaika") );

    painter->drawText(QRectF( keskiviiva + mm, pv + rk * 2, (leveys-keskiviiva) / 2, rk-mm ), Qt::AlignBottom,  asviite );

    // Kirjoittamista jatkettaan ruudukon jälkeen - taikka ikkunan, jos se on isompi
    qreal ruutukorkeus = asviite.isEmpty() ? pv + rk * 2 : pv + rk *3;
    qreal ikkunakorkeus = ikkuna.y() + ikkuna.height() + 5 * mm;

    painter->setFont( QFont("Sans", 10));
    QString lisatieto = map_.value("info").toString();

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
    return tekstit_.value(avain);
}


void MyyntiLaskunTulostaja::tekstiRivinLisays(const QString &rivi, const QString& kieli)
{

    int valinpaikka = rivi.indexOf(' ');
    if( valinpaikka > 2 && rivi.length() > 3) {
        QString avain = rivi.left(valinpaikka);
        if( avain.at(0).isUpper())
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

    painter->drawText(QRectF( mm*22, mm * 33, mm * 90, mm * 25), Qt::TextWordWrap,  map_.value("lasku").toMap().value("osoite").toString() );

    painter->drawText( QRectF(mm*133.4, mm*53.8, mm*60, mm*7.5), Qt::AlignLeft | Qt::AlignBottom, muotoiltuViite() );

    painter->drawText( QRectF(mm*133.4, mm*62.3, mm*30, mm*7.5), Qt::AlignLeft | Qt::AlignBottom, map_.value("lasku").toMap().value("erapvm").toDate().toString("dd.MM.yyyy") );
    painter->drawText( QRectF(mm*165, mm*62.3, mm*30, mm*7.5), Qt::AlignRight | Qt::AlignBottom, QString("%L1").arg( laskunSumma_ ,0,'f',2) );

    painter->drawText( QRectF(mm*22, mm*17, mm*90, mm*13), Qt::AlignTop | Qt::TextWordWrap, kp()->asetus("Nimi") + "\n" + kp()->asetus("Osoite")  );
    painter->drawText( QRectF(mm*22, 0, mm*90, mm*17), Qt::AlignVCenter ,  valeilla( iban_ )  );


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

QByteArray MyyntiLaskunTulostaja::qrSvg() const
{
    // Esitettävä tieto
    QString data("BCD\n001\n1\nSCT\n");

    QString bic = LaskutModel::bicIbanilla(iban_);
    if( bic.isEmpty())
        return QByteArray();
    data.append(bic + "\n");
    data.append(kp()->asetukset()->asetus("Nimi") + "\n");
    data.append(iban_ + "\n");
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
    qlonglong summa = qRound( laskunSumma_ * 100);

    if( summa > 99999999 )  // Ylisuuri laskunsumma
        return QString();

    QString koodi = kp()->asetukset()->onko("LaskuRF") ?
         QString("5 %1 %2 %3 %4 %5")
                .arg(iban_.mid(2,16))    // Numeerinen tilinumero
                .arg(summa, 8, 10, QChar('0'))
                .arg(muotoiltuViite().mid(2,2) )
                .arg(muotoiltuViite().remove(' ').mid(4),21,QChar('0'))
                .arg( map_.value("lasku").toMap().value("erapvm").toDate().toString("yyMMdd"))
        :
        QString("4 %1 %2 000 %3 %4")
            .arg( iban_.mid(2,16) )  // Tilinumeron numeerinen osuus
            .arg( summa, 8, 10, QChar('0') )  // Rahamäärä
            .arg( map_.value("lasku").toMap().value("viite").toString(), 20, QChar('0'))
            .arg( map_.value("lasku").toMap().value("erapvm").toDate().toString("yyMMdd"));

    return koodi.remove(QChar(' '));
}
