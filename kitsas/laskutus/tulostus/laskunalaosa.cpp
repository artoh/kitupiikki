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
#include "laskunalaosa.h"

#include "../nayukiQR/QrCode.hpp"

#include "db/kitsasinterface.h"
#include "db/asetusmodel.h"
#include <QPainter>
#include <QSvgRenderer>

#include <QDebug>

LaskunAlaosa::LaskunAlaosa(KitsasInterface *interface) :
    interface_(interface)
{
    lataaIbanit();

    viivakoodi_ = interface_->asetukset()->onko(AsetusModel::LaskuViivakoodi);
    tilisiirto_ = interface_->asetukset()->onko(AsetusModel::LaskuTilisiirto);
    virtuaaliviivakoodi_ = interface_->asetukset()->onko(AsetusModel::LaskuVirtuaaliviivakoodi);
}

void LaskunAlaosa::lataa(const Lasku &lasku, const QString &vastaanottaja)
{
    kieli_ = lasku.kieli().toLower();
    vastaanottaja_ = vastaanottaja;

    lataaYhteystiedot();
    lataaMaksutiedot(lasku);
}

qreal LaskunAlaosa::laske(QPainter *painter)
{
    double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();
    qreal leveys = painter->window().width();
    painter->setFont(QFont("FreeSans", 9));

    maksuKorkeus_ = maksulaatikko_.laske(painter,
            maksulaatikko_.sarakkeita() > 2
            ? leveys
            : leveys / 3);

    if( viivakoodi_ && !tilisiirto_)
        leveys = (leveys - 120 * mm) / 2;
    else
        leveys = leveys / 4;

    yhteysKorkeus_ = osoitelaatikko_.laskeKoko(painter, leveys);

    if( viivakoodi_ && !tilisiirto_ && maksulaatikko_.sarakkeita() > 2  ) {
        qreal yhteyslaatikkoKorkeus = yhteyslaatikko_.laskeKoko(painter, leveys) +
                                      tunnuslaatikko_.laskeKoko(painter, leveys);
        if( yhteyslaatikkoKorkeus > yhteysKorkeus_) yhteysKorkeus_ = yhteyslaatikkoKorkeus;
        if( yhteysKorkeus_ < 20 * mm)
            yhteysKorkeus_ = 20 * mm;
    } else {
        qreal yhteyslaatikkoKorkeus = yhteyslaatikko_.laskeKoko(painter, leveys);
        if( yhteyslaatikkoKorkeus > yhteysKorkeus_) yhteysKorkeus_ = yhteyslaatikkoKorkeus;
        qreal tunnuskorkeus = tunnuslaatikko_.laskeKoko(painter, leveys);
        if( tunnuskorkeus > yhteysKorkeus_) yhteysKorkeus_ = tunnuskorkeus;
    }

    alaosaKorkeus_ = maksuKorkeus_ + yhteysKorkeus_ +
        ( tilisiirto_ ? mm * 95 : mm * 5) +
        ( virtuaaliviivakoodi_ ? painter->fontMetrics().height() : 0 );

    qDebug() << " window " << painter->window().height()
             << " alaosa " << alaosaKorkeus_
             << " mm " << alaosaKorkeus_/mm;

    return alaosaKorkeus_ + mm * 5;
}

void LaskunAlaosa::piirra(QPainter *painter, const Lasku &lasku)
{
    painter->save();

    painter->resetTransform();
    painter->translate( 0, painter->window().height() - alaosaKorkeus_ );

    painter->setFont(QFont("FreeSans", 9));
    qreal leveys = painter->window().width();
    double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();


    if( virtuaaliviivakoodi_ ) {
        const QString& koodi = lasku.virtuaaliviivakoodi( ibanit_.value(0),
                                                          interface_->asetukset()->onko(AsetusModel::LaskuRF));

        const QString vvk = kaanna("virtviiv") + " " + koodi;
        QRectF vvkr = painter->boundingRect(QRect(0, 0, leveys, mm *10), vvk);
        if( !koodi.isEmpty() ) {
            painter->drawText(vvkr, vvk);
        }
        painter->translate(0, vvkr.height());        
    }

    maksulaatikko_.piirra(painter, leveys - maksulaatikko_.koko().width(), 0);
    painter->translate(0, maksuKorkeus_ + painter->fontMetrics().horizontalAdvance("ii"));

    if( viivakoodi_ && !tilisiirto_ && maksulaatikko_.sarakkeita() > 2) {
        osoitelaatikko_.piirra(painter, mm * 110, 0);

        qreal yhteyslaatikkoLeveys = yhteyslaatikko_.koko().width();
        qreal tunnuslaatikkoLeveys = tunnuslaatikko_.koko().width();
        qreal leveampi = yhteyslaatikkoLeveys > tunnuslaatikkoLeveys ?
                    yhteyslaatikkoLeveys :
                    tunnuslaatikkoLeveys;

        yhteyslaatikko_.piirra(painter, leveys - leveampi, 0);
        tunnuslaatikko_.piirra(painter, leveys - leveampi, yhteyslaatikko_.koko().height());

        piirraViivakoodi(painter, QRectF(mm*5, mm*5, mm*100, mm*13), lasku);
    } else {
        osoitelaatikko_.piirra(painter, 0, 0);
        yhteyslaatikko_.piirra(painter,
                           leveys / 3 +  (leveys / 3  -  yhteyslaatikko_.koko().width()) / 2,
                           0);
        tunnuslaatikko_.piirra(painter, leveys - tunnuslaatikko_.koko().width(), 0);
    }
    painter->translate(0,  yhteysKorkeus_ + mm * 3 );

    if( tilisiirto_) {
        piirraTilisiirto(painter, lasku);
    }
    painter->restore();

}

qreal LaskunAlaosa::alatunniste(QPainter *painter)
{
    qreal korkeus = osoitelaatikko_.koko().height();
    if( yhteyslaatikko_.koko().height() > korkeus) korkeus = yhteyslaatikko_.koko().height();
    if( tunnuslaatikko_.koko().height() > korkeus) korkeus = tunnuslaatikko_.koko().height();

    painter->save();
    painter->translate(0, painter->window().height() - korkeus - painter->transform().dy());

    qreal leveys = painter->window().width();

    osoitelaatikko_.piirra(painter, 0, 0);
    yhteyslaatikko_.piirra(painter,
                       leveys / 3 +  (leveys / 3  -  yhteyslaatikko_.koko().width()) / 2,
                       0);
    tunnuslaatikko_.piirra(painter, leveys - tunnuslaatikko_.koko().width(), 0);

    painter->setFont(QFont("FreeSans", 9));
    qreal rivinkorkeus = painter->fontMetrics().height();

    painter->restore();
    return painter->window().height() - korkeus + rivinkorkeus * 2;
}

void LaskunAlaosa::lataaIbanit()
{
    const AsetusModel* asetukset = interface_->asetukset();
    QStringList ibanLista = asetukset->asetus(AsetusModel::LaskuIbanit).split(",");

    for(const auto& ibanteksti : qAsConst( ibanLista )) {
        Iban iban(ibanteksti);
        pankit_ << iban.pankki();
        ibanit_ << iban.valeilla();
        bicit_ << iban.bic();
    }
}

QString LaskunAlaosa::kaanna(const QString &avain) const
{
    return interface_->kaanna(avain, kieli_).replace("|","\n");
}

void LaskunAlaosa::lataaYhteystiedot()
{
    const AsetusModel* asetukset = interface_->asetukset();

    lahettaja_ = asetukset->asetus(AsetusModel::OrganisaatioNimi) + "\n" +
            asetukset->asetus(AsetusModel::Katuosoite) + "\n" +
            asetukset->asetus(AsetusModel::Postinumero) + " " + asetukset->asetus(AsetusModel::Kaupunki);

    osoitelaatikko_.lisaa(QString(), lahettaja_);

    lisaaYhteys(AsetusModel::Kotisivu, "kotisivu");
    lisaaYhteys(AsetusModel::Puhelin, "puhelin");
    lisaaYhteys(AsetusModel::Email, "sahkoposti");
    lisaaYhteys(AsetusModel::Kotipaikka, "Kotipaikka");


    const QString& ytunnus = asetukset->asetus(AsetusModel::Ytunnus);
    if( !ytunnus.isEmpty())
        lisaaTunnukseen( kaanna("ytunnus"), ytunnus );
    if( asetukset->onko(AsetusModel::AlvVelvollinen))
        lisaaTunnukseen(kaanna("alvtunnus"), "FI"+ytunnus.left(7)+ytunnus.right(1));
    else
        lisaaTunnukseen(QString(), kaanna("eialv"));
}

void LaskunAlaosa::lataaMaksutiedot(const Lasku &lasku)
{
    const AsetusModel* asetukset = interface_->asetukset();

    if( lasku.maksutapa() != Lasku::KATEINEN &&
             lasku.summa().cents() > 0) {

    maksulaatikko_.lisaa( kaanna("pankki"), pankit_.join("\n"));
    maksulaatikko_.lisaa( kaanna("iban"), ibanit_.join("\n") );
    maksulaatikko_.lisaa( kaanna("bic"), bicit_.join("\n"));

    if( lasku.viite().tyyppi() != ViiteNumero::VIRHEELLINEN)
        maksulaatikko_.lisaa( kaanna("viitenro"), asetukset->onko(AsetusModel::LaskuRF)
                          ? lasku.viite().rfviite() :
                            lasku.viite().valeilla());
    } else {
        tilisiirto_ = false;
        viivakoodi_ = false;
        virtuaaliviivakoodi_ = false;
    }

    if( lasku.maksutapa() == Lasku::KATEINEN)
        maksulaatikko_.lisaa( kaanna("erapvm"), kaanna("maksettu"));
    else if( lasku.erapvm().isValid() && lasku.summa().cents() > 0)
        maksulaatikko_.lisaa( kaanna("erapvm"), lasku.erapvm().toString("dd.MM.yyyy"));


    if( lasku.maksutapa() != Lasku::KUUKAUSITTAINEN)
        maksulaatikko_.lisaa(kaanna("Yhteensa"), lasku.summa().display(), Qt::AlignRight, true);

}

void LaskunAlaosa::lisaaYhteys(const int &asetusavain, const QString &kaannosavain)
{
    const QString arvo = interface_->asetukset()->asetus(asetusavain);
    if( !arvo.isEmpty()) {
        yhteyslaatikko_.lisaa( kaanna(kaannosavain), arvo);
    }

}

void LaskunAlaosa::lisaaTunnukseen(const QString &avain, const QString &teksti)
{
    tunnuslaatikko_.lisaa( kaanna(avain), teksti );
}

void LaskunAlaosa::piirraTilisiirto(QPainter *painter, const Lasku &lasku)
{
    painter->save();
    painter->setFont(QFont("FreeSans", 7));
    double mm = painter->device()->width() * 1.00 / painter->device()->widthMM();

    // QR-koodi
    if( interface_->asetukset()->onko(AsetusModel::LaskuQR))
    {
        QString data = lasku.QRkooditieto( ibanit_.value(0),
                                           interface_->asetukset()->asetus(AsetusModel::OrganisaatioNimi),
                                           interface_->asetukset()->onko(AsetusModel::LaskuRF) );
        if( !data.isEmpty() ) {
            qrcodegen::QrCode qrTieto = qrcodegen::QrCode::encodeText( data.toUtf8().data() , qrcodegen::QrCode::Ecc::QUARTILE);
            QSvgRenderer qrr( QByteArray::fromStdString( qrTieto.toSvgString(1) ) );
            qrr.render( painter, QRectF( ( painter->device()->widthMM() - 35 ) *mm, 5 * mm, 30 * mm, 30 * mm  ) );
        }
    }

    double loppu = painter->window().width();
    double pv = (loppu - 20 * mm) / 2 + 20 * mm;
    double osle = (painter->window().width() - 20 * mm) / 2;
    double viervi = pv + 20 * mm;
    double eurv = (painter->window().width() - 20 * mm) * 3 / 4 + 20 * mm;

    painter->drawText( QRectF(0,0,mm*19,mm*16.9), Qt::AlignRight | Qt::AlignHCenter, kaanna("bst"));
    painter->drawText( QRectF(0, mm*18, mm*19, mm*14.8), Qt::AlignRight | Qt::AlignHCenter, kaanna("bsa"));
    painter->drawText( QRectF(0, mm*32.7, mm*19, mm*20), Qt::AlignRight | Qt::AlignTop, kaanna("bmo"));
    painter->drawText( QRectF(0, mm*51.3, mm*19, mm*10), Qt::AlignRight | Qt::AlignVCenter , kaanna("bak"));
    painter->drawText( QRectF(0, mm*62.3, mm*19, mm*8.5), Qt::AlignRight | Qt::AlignHCenter, kaanna("btl"));
    painter->drawText( QRectF(mm * 22, 0, mm*20, mm*10), Qt::AlignLeft, kaanna("iban"));
    painter->drawText( QRectF(pv + mm * 2, 0, mm*20, mm*10), Qt::AlignLeft, kaanna("bic"));

    painter->drawText( QRectF(pv + 2 * mm, mm*53.8, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, kaanna("bvn"));
    painter->drawText( QRectF(pv + 2 * mm, mm*62.3, mm*15, mm*8.5), Qt::AlignLeft | Qt::AlignTop, kaanna("bep"));
    painter->drawText( QRectF(eurv + 2 * mm, mm*62.3, mm*19, mm*8.5), Qt::AlignLeft, kaanna("eur"));

    painter->setFont(QFont("FreeSans",6));
    painter->drawText( QRectF( loppu - mm * 60, mm * 72, mm * 60, mm * 20), Qt::AlignLeft | Qt::TextWordWrap, kaanna("behto") );
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

    painter->drawText(QRectF( mm*22, mm * 33, osle, mm * 25), Qt::TextWordWrap,  vastaanottaja_ );

    painter->drawText( QRectF(viervi + 2 * mm, mm * 53.8 ,loppu - viervi - 4*mm, mm*7.5), Qt::AlignLeft | Qt::AlignVCenter,
                       interface_->asetukset()->onko(AsetusModel::LaskuRF) ? lasku.viite().rfviite() : lasku.viite().valeilla() );

    painter->drawText( QRectF(viervi + 2 * mm , mm*62.3, eurv - viervi - 4 * mm , mm*7.5), Qt::AlignLeft | Qt::AlignVCenter, lasku.erapvm().toString("dd.MM.yyyy") );
    painter->drawText( QRectF(eurv, mm*62.3, loppu - eurv - 10 * mm, mm*7.5), Qt::AlignRight | Qt::AlignVCenter, lasku.summa().display() );

    painter->drawText( QRectF(mm*22, mm*17, osle, mm*13), Qt::AlignTop | Qt::TextWordWrap, lahettaja_  );

    painter->drawText( QRectF(mm*22, 0, osle, mm*17), Qt::AlignVCenter, ibanit_.join("\n") );
    painter->drawText( QRectF(pv+2*mm, 0, osle, mm*17), Qt::AlignVCenter, bicit_.join("\n") );

    painter->save();
    painter->setFont(QFont("FreeSans", 7));
    painter->translate(mm * 2, mm* 60);
    painter->rotate(-90.0);
    painter->drawText(0,0,kaanna("btilis"));
    painter->restore();

    // Viivakoodi
    if( viivakoodi_ )
    {
        piirraViivakoodi(painter, QRectF( mm*10, mm*72, mm*100, mm*13), lasku);
    }
    painter->restore();
}

void LaskunAlaosa::piirraViivakoodi(QPainter *painter, const QRectF &rect, const Lasku &lasku)
{
    painter->save();

    QFont koodifontti( "code128_XL", 36);
    koodifontti.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
    painter->setFont( koodifontti);
    QString koodi( code128( lasku.virtuaaliviivakoodi( ibanit_.value(0),
                                                       interface_->asetukset()->onko(AsetusModel::LaskuRF)) ) );
    painter->drawText( rect, Qt::AlignCenter, koodi  );

    painter->restore();

}

QString LaskunAlaosa::code128(const QString &koodattava)
{
    QString koodi;
    koodi.append( QChar(210) );   // Code C aloitusmerkki

    int summa = 105;
    int paino = 1;

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

QChar LaskunAlaosa::code128c(int koodattava)
{
    if( koodattava < 95)
        return QChar( 32 + koodattava);
    else
        return QChar( 105 + koodattava);
}

