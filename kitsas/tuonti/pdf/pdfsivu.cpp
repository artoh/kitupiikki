#include "pdfsivu.h"

#include <QPdfDocument>
#include <iostream>

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

void PdfSivu::tuo(QPdfDocument *doc, int sivu)
{
    sivunKoko_ = doc->pagePointSize(sivu).toSize();

    QPdfSelection sel = doc->getAllText(sivu);
    QString text = sel.text();

    int start = 0;
    for(int c=0; c < text.length(); c++) {
        if( text.at(c).isSpace()) {
            if(start >= c) {
                start = c+1;
                continue;
            }
            QPdfSelection piece = doc->getSelectionAtIndex(sivu, start, c - start);
            lisaa(piece);
            start = c+1;
        }
    }
    for(int i=0; i < rivit_.count(); i++)
        rivit_[i]->yhdistaPalat();

}

QString PdfSivu::teksti() const
{
    QString ulos;
    for(const auto rivi : qAsConst(rivit_)) {
        ulos.append( rivi->teksti() + "\n" );
    }
    return ulos;
}

void PdfSivu::lisaa(const QPdfSelection &selection)
{
    for(int i=0; i < rivit_.count(); i++) {
        int vertaus = rivit_[i]->vertaa(selection);
        if( vertaus < 0 ) {
            rivit_.insert(i, new PdfRivi(selection));
            return;
        }
        if( vertaus == 0) {
            rivit_[i]->tuo(selection);
            return;
        }
    }
    rivit_.append( new PdfRivi(selection) );
}

} // namespace Tuonti
