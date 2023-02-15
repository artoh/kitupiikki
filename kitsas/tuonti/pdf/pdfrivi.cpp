#include "pdfrivi.h"
#include <QRegularExpression>

namespace Tuonti {

PdfRivi::PdfRivi()
{

}

PdfRivi::PdfRivi(const QPdfSelection &selection)
{
    ekapala_ = new PdfPala(selection);
    vikapala_ = ekapala_;
    alue_ = selection.boundingRectangle().toRect();
}

PdfRivi::~PdfRivi()
{
    if(ekapala_)
        delete ekapala_;
}

int PdfRivi::vertaa(const QPdfSelection &selection) const
{
    const int ylareuna = selection.boundingRectangle().top();
    const int alareuna = selection.boundingRectangle().bottom();
    const int neljannes = ekapala_->korkeus();

    if( ylareuna == ekapala_->yla() && alareuna == ekapala_->ala()) return 0;

    if( alareuna < ekapala_->yla())
        return -1;
    else if( ylareuna > ekapala_->ala())
        return 1;

    if( alareuna < ekapala_->ala() - neljannes ||
        ylareuna < ekapala_->yla() - neljannes)
        return -1;
    if( ylareuna > ekapala_->yla() + neljannes ||
        alareuna > ekapala_->ala() + neljannes   )
        return 1;

    PdfPala* pala = ekapala_->seuraava();
    while(pala) {
        if( pala->yla() >= alareuna)
            return -1;
        else if(pala->ala() <= ylareuna)
            return 1;
        pala = pala->seuraava();
    }


    return 0;       // KELPAA ;)
}

void PdfRivi::tuo(const QPdfSelection &selection)
{
    int vasen = selection.boundingRectangle().left();

    PdfPala* uusi = new PdfPala(selection);
    alue_ = alue_.united(uusi->rect());

    if( vasen > vikapala_->vasen()) {
        vikapala_->asetaSeuraava(uusi);
        vikapala_ = uusi;
        return;
    }

    if( vasen < ekapala_->vasen()) {        
        uusi->asetaSeuraava( ekapala_ );
        ekapala_ = uusi;
    } else {
        PdfPala* pala = ekapala_;
        while( pala->vasen() > vasen && pala->seuraava())
            pala = pala->seuraava();        
        uusi->asetaSeuraava(pala->seuraava());
        pala->asetaSeuraava( uusi );

    }
}

void PdfRivi::yhdistaPalat()
{
    PdfPala* pala = ekapala_;
    while(pala && pala->seuraava()) {                
        const int ero = pala->kirjainLeveys() > 7 ? pala->kirjainLeveys() : 8;
        const int rako = pala->seuraava()->vasen() - pala->oikea();
        if(  rako < ero  ||
            (pala->vasen() > 500 && rahaOsaRe__.match(pala->teksti()).hasMatch() && rako < 25)) {
            pala->seuraava()->yhdistaEdelliseen(pala);            
        } else {
            pala = pala->seuraava();
        }
    }

    // Yhdistetään rivin lopettava + tai - edelliseen
    pala = vikapala_;
    if( pala->teksti() == "+" || pala->teksti() == "-") {
        PdfPala* edellinen = ekapala_;
        while( edellinen->seuraava() != pala) {
            edellinen = edellinen->seuraava();
            if(!edellinen) return;
        }
        pala->yhdistaEdelliseen(edellinen);
    }

}

int PdfRivi::paloja() const
{
    int lkm = 0;
    PdfPala* pala = ekapala_;
    while(pala) {
        lkm++;
        pala = pala->seuraava();
    }
    return lkm;
}

bool PdfRivi::sisaltaako(const QStringList &tekstit) const
{
    PdfPala* pala = ekapala_;
    while(pala) {
        if( pala->sisaltaako(tekstit)) {
            return true;
        }
        pala = pala->seuraava();
    }
    return false;
}

QString PdfRivi::teksti() const
{
    QString ulos;
    PdfPala* pala = ekapala_;
    QString fsi;

    int kirjainLeveys = ekapala_->kirjainLeveys() > 7 ? ekapala_->kirjainLeveys() : 8;

    if(pala->kirjainLeveys())
        fsi.fill(' ', pala->vasen() / kirjainLeveys);
    ulos.append(fsi);
    while(pala) {
        ulos.append( pala->teksti());
        if( pala->seuraava()) {
            QString fs;
            if( pala->kirjainLeveys())
                fs.fill(' ', ((pala->seuraava()->vasen() - pala->oikea()) / kirjainLeveys) + 1);
            ulos.append(fs);
        }
        pala = pala->seuraava();
    }
    return ulos;
}

QRect PdfRivi::alue() const
{
    return alue_;
}

QRegularExpression PdfRivi::rahaOsaRe__("[+-]?(\\s*\\d{1,3})+");

} // namespace Tuonti
