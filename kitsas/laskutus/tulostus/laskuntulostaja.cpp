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
#include "laskuntulostaja.h"

#include <QPainter>
#include <QPagedPaintDevice>

#include <QPdfWriter>
#include <QBuffer>

#include <QApplication>

#include "db/kitsasinterface.h"
#include "laskuntietolaatikko.h"
#include "laskunosoitealue.h"
#include "laskuruudukontayttaja.h"

#include "db/yhteysmodel.h"
#include "db/tositetyyppimodel.h"

LaskunTulostaja::LaskunTulostaja(KitsasInterface *kitsas, QObject *parent)
    : QObject(parent), kitsas_(kitsas), tietoLaatikko_(kitsas), alaOsa_(kitsas)
{

}

void LaskunTulostaja::tulosta(Tosite &tosite, QPagedPaintDevice *printer, QPainter *painter)
{
    painter->resetTransform();

    const Lasku& lasku = tosite.constLasku();
    kieli_ = lasku.kieli().toLower();
    varjoVari_ = kitsas_->toiminimet()->vari( ToiminimiModel::VariVarjo, lasku.toiminimi());

    if( lasku.numero().isEmpty())
        tulostaLuonnos(painter);

    painter->setFont(QFont("FreeSans", 10));
    qreal rivinkorkeus = painter->fontMetrics().height();


    LaskunOsoiteAlue osoiteosa( kitsas_ );    


    osoiteosa.lataa(tosite);
    tietoLaatikko_.lataa(tosite);
    alaOsa_.lataa(lasku, osoiteosa.vastaanottaja());

    qreal alalaita = painter->window().height() - alaOsa_.laske(painter) - rivinkorkeus * 2;

    alaOsa_.piirra(painter, lasku);

    osoiteosa.laske(painter, printer);

    qreal sivunleveys = painter->window().width();
    qreal laatikkoleveys = osoiteosa.leveys() > sivunleveys
            ? sivunleveys - osoiteosa.leveys()
            : sivunleveys / 2;
    tietoLaatikko_.laskeLaatikko(painter, laatikkoleveys);

    painter->save();
    osoiteosa.piirra(painter);
    painter->translate(sivunleveys - laatikkoleveys, 0);
    tietoLaatikko_.piirra(painter);
    painter->restore();

    painter->translate(0, osoiteosa.korkeus() > tietoLaatikko_.korkeus() ?
                          osoiteosa.korkeus() : tietoLaatikko_.korkeus());
    painter->translate( 0, rivinkorkeus * 0.5 );

    if( !lasku.otsikko().isEmpty()) {
        painter->setFont(QFont("FreeSans", 10, QFont::Bold));
        QRectF oRect = painter->boundingRect( QRectF(0,0,sivunleveys,sivunleveys),
                                                 Qt::TextWordWrap,
                                                 lasku.otsikko());
        painter->drawText( oRect, Qt::TextWordWrap, lasku.otsikko() );
        painter->translate(0, oRect.height());
        painter->translate( 0, painter->fontMetrics().height() * 0.5 );
    }

    if( !lasku.lisatiedot().isEmpty() ) {
        const QString lisatiedot = lasku.tulkkaaMuuttujat( lasku.lisatiedot() );
        painter->setFont(QFont("FreeSans", 10));
        QRectF lisaRect = painter->boundingRect( QRectF(0,0,sivunleveys,sivunleveys),
                                                 Qt::TextWordWrap,
                                                 lisatiedot);
        painter->drawText( lisaRect, Qt::TextWordWrap, lisatiedot );
        painter->translate(0, lisaRect.height());
        painter->translate( 0, painter->fontMetrics().height() * 0.5 );
    }

    // Sitten pitäisi alkaa tulostaa riveja niin paljon kun mahtuu
    alalaita = tulostaRuudukko(tosite, painter, printer, alalaita);

    // Maksumuikkarille aiempia
    if( tosite.tyyppi() == TositeTyyppi::MAKSUMUISTUTUS)
        alalaita = muistutettavatLaskut(tosite, painter, printer, alalaita);

    tulostaErittely( lasku.erittely(), painter, printer, alalaita);
}

QByteArray LaskunTulostaja::pdf(Tosite &tosite)
{
    QByteArray array;
    QBuffer buffer(&array);
    buffer.open( QIODevice::WriteOnly);

    QPdfWriter writer(&buffer);
    writer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
    writer.setPageSize( QPageSize(QPageSize::A4));
    writer.setPageMargins( QMarginsF(10,10,10,10), QPageLayout::Millimeter );
    QPainter painter(&writer);

    writer.setCreator(QString("%1 %2").arg( qApp->applicationName(), qApp->applicationVersion() ));
    writer.setTitle( tr("Lasku %1").arg( tosite.constLasku().numero() ) );
    tulosta(tosite , &writer, &painter);
    painter.end();

    buffer.close();

    return array;
}

void LaskunTulostaja::tallennaLaskuLiite(Tosite &tosite)
{
    const QByteArray lasku = pdf(tosite);

    KpKysely *tallennus = kitsas_->yhteysModel()->kysely( QString("/liitteet/%1/lasku").arg(tosite.id()), KpKysely::PUT );
    QMap<QString,QString> meta;
    meta.insert("Filename", QString("lasku%1.pdf").arg(tosite.lasku().numero()));
    meta.insert("Content-type", "application/pdf");

    connect( tallennus, &KpKysely::vastaus, this, &LaskunTulostaja::laskuLiiteValmis);
    tallennus->lahetaTiedosto(lasku, meta);    
}

void LaskunTulostaja::laskuLiiteValmis()
{
    emit laskuLiiteTallennettu();
    deleteLater();
}


void LaskunTulostaja::tulostaLuonnos(QPainter *painter)
{
    painter->save();
    painter->setPen( QPen( Qt::lightGray));
    painter->setFont( QFont("FreeSans",60,QFont::Black));
    painter->drawText(QRect( 0, 0, painter->window().width(), painter->window().height() ), Qt::AlignCenter,
                      kitsas_->kaanna("luonnos", kieli_) );
    painter->restore();
}

qreal LaskunTulostaja::tulostaRuudukko(Tosite &tosite, QPainter *painter, QPagedPaintDevice *device, qreal alalaita, bool tulostaKuukaudet)
{
    // Sitten pitäisi alkaa tulostaa riveja niin paljon kun mahtuu

    LaskuRuudukonTayttaja tayttaja( kitsas_ );
    TulostusRuudukko riviosa = tayttaja.tayta(tosite);
    riviosa.asetaVarjo(varjoVari_);
    riviosa.asetaLeveys(painter->window().width(), painter->window().width());
    alalaita = riviosa.piirra(painter, device,
                   alalaita, this);

    qreal sivunleveys = painter->window().width();
    painter->setFont(QFont("FreeSans", 10));
    qreal rivinkorkeus = painter->fontMetrics().height();

    TulostusRuudukko veroRuudukko = tayttaja.alvRuudukko(painter);
    veroRuudukko.asetaVarjo(varjoVari_);
    veroRuudukko.asetaLeveys( sivunleveys / 4, sivunleveys * 3 / 4 );

    veroRuudukko.asetaPistekoko(8);

    if( painter->transform().dy() + veroRuudukko.koko().height() >= alalaita) {
        alalaita = vaihdaSivua(painter, device);
        veroRuudukko.piirra(painter, device, alalaita, this);
    } else {
        painter->translate(0, rivinkorkeus - riviosa.summaKoko().height() );
        veroRuudukko.piirra(painter, device);
        if( (riviosa.summaKoko().height() ) > veroRuudukko.koko().height() ) {
            painter->translate(0, riviosa.summaKoko().height() - veroRuudukko.koko().height() + rivinkorkeus);
        }
    }
    painter->translate(0, 1.5 * rivinkorkeus);
    const Lasku& lasku = tosite.constLasku();

    if( lasku.maksutapa() == Lasku::KUUKAUSITTAINEN && tulostaKuukaudet) {
        TulostusRuudukko kuukaudet = tayttaja.kuukausiRuudukko(lasku, painter);
        kuukaudet.asetaVarjo(varjoVari_);
        if( painter->transform().dy() + kuukaudet.koko().height() >= alalaita )
            alalaita = vaihdaSivua(painter, device);
        alalaita = kuukaudet.piirra(painter, device, alalaita, this);
        painter->translate(0, 1.5 * rivinkorkeus);
    }


    return alalaita;
}

qreal LaskunTulostaja::muistutettavatLaskut(Tosite &tosite, QPainter *painter, QPagedPaintDevice *device, qreal alalaita)
{
    qreal sivunleveys = painter->window().width();

    QVariantList aiemmat = tosite.lasku().aiemmat();
    for(const auto& item : aiemmat) {
        QVariantMap aiempiMap = item.toMap();
        Tosite aiempiTosite;
        aiempiTosite.lataa(aiempiMap);

        QString teksti = kitsas_->kaanna( aiempiTosite.tyyppi() == TositeTyyppi::MAKSUMUISTUTUS ?
                                              "aiempimuistutus" : "alkuplasku", kieli_)
                .arg( aiempiTosite.lasku().numero() )
                .arg( aiempiTosite.lasku().laskunpaiva().toString("dd.MM.yyyy") )
                .arg( aiempiTosite.erapvm().toString("dd.MM.yyyy")).replace("|","\n");

        painter->setFont(QFont("FreeSans", 10));
        QRectF trect = painter->boundingRect(QRectF(0, 0, sivunleveys, painter->fontMetrics().height() * 8), teksti );
        if( painter->transform().dy() + trect.height() > alalaita - painter->fontMetrics().height() * 6 )
            alalaita = vaihdaSivua(painter, device);
        painter->drawText( trect, teksti );
        painter->translate(0, trect.height() + painter->fontMetrics().height() * 1.5);

        alalaita = tulostaRuudukko(aiempiTosite, painter, device, alalaita, false);
    }
    return alalaita;
}


qreal LaskunTulostaja::vaihdaSivua(QPainter *painter, QPagedPaintDevice *device)
{
    painter->save();
    painter->setFont( QFont("FreeSans",10));
    QRectF jatkuuRect(0, 0, painter->window().width(), painter->fontMetrics().height() );
    painter->drawText(jatkuuRect, Qt::AlignRight, kitsas_->kaanna("jatkuu", kieli_));
    painter->restore();
    device->newPage();
    painter->resetTransform();
    tietoLaatikko_.ylatunniste(painter);
    return alaOsa_.alatunniste(painter);
}

qreal LaskunTulostaja::tulostaErittely(const QStringList &erittely, QPainter *painter, QPagedPaintDevice *device, qreal alalaita)
{
    painter->setFont(QFont("FreeMono", 9));
    QRectF oRect(0, 0, painter->window().width(), painter->fontMetrics().height());

    for( const auto& rivi : erittely) {
        if( painter->transform().dy() > alalaita)
            alalaita = vaihdaSivua(painter, device);
        painter->drawText(oRect, rivi);
        painter->translate(0, oRect.height());
    }        

    return alalaita;
}
