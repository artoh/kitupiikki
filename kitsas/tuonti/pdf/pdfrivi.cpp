#include "pdfrivi.h"

namespace Tuonti {

PdfRivi::PdfRivi()
{

}

PdfRivi::PdfRivi(const QPdfSelection &selection)
{
    ekapala_ = new PdfPala(selection);
}

PdfRivi::~PdfRivi()
{
    if(ekapala_)
        delete ekapala_;
}

int PdfRivi::vertaa(const QPdfSelection &selection) const
{
    if( selection.boundingRectangle().bottom() < ekapala_->yla())
        return -1;
    else if( selection.boundingRectangle().top() > ekapala_->ala())
        return 1;

    if( selection.boundingRectangle().bottom() < ekapala_->ala() - ekapala_->korkeus() / 3 ||
        selection.boundingRectangle().top() < ekapala_->yla() - ekapala_->korkeus() / 3)
        return -1;
    if( selection.boundingRectangle().top() > ekapala_->yla() + ekapala_->korkeus() / 3 ||
        selection.boundingRectangle().bottom() > ekapala_->ala() + ekapala_->korkeus() / 3    )
        return 1;

    return 0;       // KELPAA ;)
}

void PdfRivi::tuo(const QPdfSelection &selection)
{
    int vasen = selection.boundingRectangle().left();
    if( vasen < ekapala_->vasen()) {
        PdfPala* uusi = new PdfPala(selection);
        uusi->asetaSeuraava( ekapala_ );
        ekapala_ = uusi;
    } else {
        PdfPala* pala = ekapala_;
        while( pala->vasen() < vasen && pala->seuraava())
            pala = pala->seuraava();
        pala->asetaSeuraava( new PdfPala(selection) );
    }
}

void PdfRivi::yhdistaPalat()
{

    PdfPala* pala = ekapala_;
    while(pala && pala->seuraava()) {
        if( qAbs( pala->seuraava()->vasen() - pala->oikea()) < pala->korkeus() * 2 ) {
            pala->seuraava()->yhdistaEdelliseen(pala);
        }
        pala = pala->seuraava();
    }
}

QString PdfRivi::teksti() const
{
    QString ulos;
    PdfPala* pala = ekapala_;
    QString fsi;

    int kirjainLeveys = ekapala_->kirjainLeveys() ? ekapala_->kirjainLeveys() : 8;

    if(pala->kirjainLeveys())
        fsi.fill(' ', pala->vasen() / kirjainLeveys);
    ulos.append(fsi);
    while(pala) {
        ulos.append( pala->teksti());
        if( pala->seuraava()) {
            QString fs;
            if( pala->kirjainLeveys())
                fs.fill('_', (pala->seuraava()->vasen() - pala->oikea()) / kirjainLeveys);
            ulos.append(fs);
        }
        pala = pala->seuraava();
    }
    return ulos;
}

} // namespace Tuonti
