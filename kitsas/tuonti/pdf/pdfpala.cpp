#include "pdfpala.h"

#include <QRegularExpression>

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

bool PdfPala::sisaltaako(const QStringList &tekstit) const
{
    for(const auto& teksti : tekstit) {
        if( teksti_.contains(teksti, Qt::CaseInsensitive)) return true;
    }
    return false;
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
    edellinen->teksti_.append(" " + teksti_);
}

void PdfPala::asetaSeuraava(PdfPala *seuraava)
{
    seuraava_ = seuraava;
}

void PdfPala::etsiAlapala(PdfPala *ehdokas)
{
    if( qAbs(ehdokas->vasen() - vasen_) < 12 &&  ala_ - ehdokas->yla() < 8) {
        alla_ = ehdokas;
        return;
    }

    while( ehdokas->seuraava()) {
        ehdokas = ehdokas->seuraava();
        if( qAbs(ehdokas->vasen() - vasen_) < 12 &&  ala_ - ehdokas->yla() < 8) {
            alla_ = ehdokas;
            return;
        }
    }

    if(seuraava_)
        seuraava_->etsiAlapala(ehdokas);
}

QRect PdfPala::rect() const
{
    return QRect(QPoint(vasen(), yla()), QPoint(oikea(), ala()));
}



} // namespace Tuonti


