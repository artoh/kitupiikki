#include "pdftiedosto.h"

#include <QPdfDocument>

#include "pdflaskuntuoja.h"
#include "tuonti/pdftiliote/pdftiliotetuonti.h"

namespace Tuonti {

PdfTiedosto::PdfTiedosto(QPdfDocument *doc) :
    doc_{doc}
{

}

PdfTiedosto::~PdfTiedosto()
{
    for(auto ptr : sivut_)
        delete ptr;
}

void PdfTiedosto::lueEnsimmainenSivu(const QPdfSelection selection, bool rivit)
{
    PdfSivu* sivu = new PdfSivu();
    sivu->tuo(doc_, 0, selection, rivit);
    sivut_.append(sivu);

}

void PdfTiedosto::lueLoputSivut()
{
    for(int i=1; i < doc_->pageCount(); i++) {
        PdfSivu* sivu = new PdfSivu();
        sivu->tuo(doc_, i, doc_->getAllText(i));
        sivut_.append(sivu);
    }
}

const QVariantMap PdfTiedosto::tuo(const TuontiApuInfo &info)
{
    QStringList hyvariTekstit = QStringList() << "hyvityslasku";
    QStringList tilioteTekstit = QStringList() << "tiliote" << "kontoutdrag" << "account statement";
    QStringList laskuTekstit = QStringList() << "lasku" << "faktura" << "invoice" << "kuitti" << "kvitto";

    QPdfSelection sel = doc_->getAllText(0);


    if( findText(hyvariTekstit, sel.text()) ) return QVariantMap();
    else if( findText(tilioteTekstit, sel.text()) ) {
        lueEnsimmainenSivu(sel);
        lueLoputSivut();
        // Tuodaan tiliote
        PdfTilioteTuonti tuoja;
        return tuoja.tuo(this);
    }
    else if( findText(laskuTekstit, sel.text())) {
        // Tuodaan lasku
        lueEnsimmainenSivu(sel);
        PdfLaskunTuoja tuoja(this, info);
        return tuoja.tuo();
    }
    else return QVariantMap();

}

PdfSivu *PdfTiedosto::ekasivu()
{
    return sivut_.value(0);
}

int PdfTiedosto::sivumaara() const
{
    return sivut_.count();
}

PdfSivu *PdfTiedosto::sivu(int sivu)
{
    return sivut_.at(sivu);
}

QString PdfTiedosto::kokoTeksti() const
{
    QString ulos;
    for(const auto ptr : sivut_) {
        ulos.append(ptr->teksti());
    }
    return ulos;
}

QList<PdfPala *> PdfTiedosto::etsiPalat(const QStringList &tekstit)
{
    QList<PdfPala*> palat;
    for(const auto ptr: sivut_) {
        palat.append( ptr->etsiPalat(tekstit) );
    }
    return palat;
}

bool PdfTiedosto::findText(const QStringList &list, const QString &target)
{
    for(const auto& text : list) {
        if( target.contains(text, Qt::CaseInsensitive)) return true;
    }
    return false;
}

QRegularExpression PdfTiedosto::ibanRe__("FI\\d{2}[\\w\\s]{6,34}");


} // namespace Tuonti
