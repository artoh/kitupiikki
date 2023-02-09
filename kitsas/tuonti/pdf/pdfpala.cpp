#include "pdfpala.h"

namespace Tuonti {

PdfPala::PdfPala()
{

}

PdfPala::PdfPala(PdfPala *kopioitava) :
    yla_{kopioitava->yla()},
    ala_{kopioitava->ala()},
    vasen_{kopioitava->vasen()},
    oikea_{kopioitava->oikea()},
    kirjainLeveys_{kopioitava->kirjainLeveys()},
    teksti_{kopioitava->teksti()}
{

}

PdfPala::PdfPala(const QPdfSelection &selection) :
    yla_{(int) selection.boundingRectangle().top()},
    ala_{(int) selection.boundingRectangle().bottom()},
    vasen_{(int) selection.boundingRectangle().left()},
    oikea_{(int) selection.boundingRectangle().right()},
    teksti_{selection.text()}
{

    if(teksti_.length()) {
        kirjainLeveys_ = (oikea_ - vasen_) / teksti_.length();
    }

}

PdfPala::~PdfPala()
{
    if(osapala_)
        delete osapala_;
    if(seuraava_)
        delete seuraava_;
}

void PdfPala::lisaaJalkeen(PdfPala *lisattava)
{
    lisattava->seuraava_ = seuraava_;
    seuraava_ = lisattava;
}

void PdfPala::yhdistaEdelliseen(PdfPala *edellinen)
{
    if( !edellinen->osapala()) {
        PdfPala* ekaosa = new PdfPala(edellinen);
        edellinen->osapala_ = ekaosa;
    }
    edellinen->osapala_->seuraava_ = this;
    edellinen->seuraava_ = seuraava_;
    seuraava_ = nullptr;

    edellinen->oikea_ = oikea_;
    edellinen->teksti_.append("=" + teksti_);
}

void PdfPala::asetaSeuraava(PdfPala *seuraava)
{
    seuraava_ = seuraava;
}


} // namespace Tuonti
