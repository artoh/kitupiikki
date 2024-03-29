#include "pdfsivu.h"

#include <QPdfDocument>
#include <iostream>

#include <QDateTime>

namespace Tuonti {

PdfSivu::PdfSivu()
{

}

PdfSivu::~PdfSivu()
{
    for(auto ptr : rivit_) {
        delete ptr;
    }
}

void PdfSivu::tuo(QPdfDocument *doc, int sivu, const QPdfSelection& selection, bool rivit)
{
    sivunKoko_ = doc->pagePointSize(sivu).toSize();

    QString text = selection.text();

//    qDebug() << sel.text();
    const QDateTime startTime = QDateTime::currentDateTime();

    int start = 0;
    for(int c=0; c < text.length(); c++) {
        if( rivit ? text.at(c) == '\n' : text.at(c).isSpace()) {
            if(start >= c) {
                start = c+1;
                continue;
            }            

            QPdfSelection piece = doc->getSelectionAtIndex(sivu, start, c - start);
            lisaa(new PdfPala(piece));
            start = c+1;
        }
    }  
  qDebug() << "E  " << sivu << "  " << startTime.msecsTo(QDateTime::currentDateTime());


    for(int i=0; i < rivit_.count(); i++)
        rivit_[i]->yhdistaPalat();
    for(int i=0; i < rivit_.count() - 1; i++)
        rivit_[i]->pala()->etsiAlapala(  rivit_[i+1]->pala() );

//    qDebug() << teksti();

}

QString PdfSivu::teksti() const
{
    QString ulos;
    for(const auto rivi : qAsConst(rivit_)) {
        ulos.append( rivi->teksti() + "\n" );
    }
    return ulos;
}

int PdfSivu::etsi(const QStringList &tekstit, int maxRivi)
{
    const int saakka = maxRivi && maxRivi < rivit_.count() ? maxRivi : rivit_.count();

    for(int i=0; i < saakka; i++) {
        if( rivit_.at(i)->sisaltaako(tekstit))
            return i+1;
    }
    return 0;
}

QList<PdfPala *> PdfSivu::etsiPalat(const QStringList &tekstit, const QRect alue)
{
    QList<PdfPala*> palat;
    for( auto rivi : rivit_) {
        if( alue.isValid() && !alue.intersects(rivi->alue())) continue;
        PdfPala* pala = rivi->pala();
        while( pala ) {
            if( (alue.isNull() || alue.intersects(pala->rect())) &&
                 pala->sisaltaako(tekstit)) {
                palat.append(pala);
            }
            pala = pala->seuraava();
        }
    }
    return palat;
}

QList<PdfPala *> PdfSivu::etsiPalat(const QRect alue)
{
    QList<PdfPala*> palat;
    for( auto rivi : rivit_) {
        if( !alue.intersects(rivi->alue())) continue;
        PdfPala* pala = rivi->pala();
        while( pala ) {
            if( alue.intersects(pala->rect())) {
                palat.append(pala);
            }
            pala = pala->seuraava();
        }
    }
    return palat;
}

PdfPala *PdfSivu::etsiPala(const QString &teksti, const QRect &alue)
{
    for( auto rivi : rivit_) {
        if( !alue.intersects(rivi->alue())) continue;
        PdfPala* pala = rivi->pala();
        while( pala ) {
            if( alue.intersects(pala->rect()) && pala->teksti().contains(teksti, Qt::CaseInsensitive)) {
                return pala;
            }
            pala = pala->seuraava();
        }
    }
    return nullptr;
}

int PdfSivu::riveja() const
{
    return rivit_.count();
}

PdfRivi *PdfSivu::rivi(int rivi)
{
    return rivit_.at(rivi);
}

void PdfSivu::lisaa(PdfPala *uusipala)
{
    if( rivit_.isEmpty()) {
        rivit_.append( new PdfRivi(uusipala) );
        return;
    }

    int lvertaus = rivit_.last()->vertaa(uusipala);
    if( lvertaus == 0) {
        rivit_.last()->tuo(uusipala);
        return;
    } else if( lvertaus > 0) {
        rivit_.append(new PdfRivi(uusipala));
        return;
    }


    for(int i=0; i < rivit_.count(); i++) {
        int vertaus = rivit_[i]->vertaa(uusipala);
        if( vertaus < 0 ) {
            rivit_.insert(i, new PdfRivi(uusipala));
            return;
        }
        if( vertaus == 0) {
            rivit_[i]->tuo(uusipala);
            return;
        }
    }
    rivit_.append( new PdfRivi(uusipala) );
}

} // namespace Tuonti
