#include "pdftiedosto.h"

#include <QPdfDocument>

#include "pdflaskuntuoja.h"
#include "tuonti/pdftiliote/pdftiliotetuonti.h"

namespace Tuonti {

PdfTiedosto::PdfTiedosto(QPdfDocument *doc) :
    doc_{doc}
{
    for(int i=0; i < doc->pageCount(); i++) {
        PdfSivu* sivu = new PdfSivu();
        sivu->tuo(doc, i);
        sivut_.append(sivu);
    }
}

PdfTiedosto::~PdfTiedosto()
{
    for(auto ptr : sivut_)
        delete ptr;
}

const QVariantMap PdfTiedosto::tuo(const TuontiApuInfo &info)
{
    QStringList hyvariTekstit = QStringList() << "hyvityslasku";
    QStringList tilioteTekstit = QStringList() << "tiliote" << "kontoutdrag" << "account statement";
    QStringList laskuTekstit = QStringList() << "lasku" << "faktura" << "invoice" << "kuitti" << "kvitto";

    PdfSivu* ekaSivu = sivut_.value(0);
    if(!ekaSivu) return QVariantMap();

    if( ekaSivu->etsi(hyvariTekstit, 10)) return QVariantMap();
    else if(ekaSivu->etsi(tilioteTekstit)) {
        // Tuodaan tiliote
        PdfTilioteTuonti tuoja;
        return tuoja.tuo(this);
    }
    else if(ekaSivu->etsi(laskuTekstit)) {
        // Tuodaan lasku
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

QRegularExpression PdfTiedosto::ibanRe__("FI\\d{2}[\\w\\s]{6,34}");


} // namespace Tuonti
