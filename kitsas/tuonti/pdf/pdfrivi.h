#ifndef TUONTI_PDFRIVI_H
#define TUONTI_PDFRIVI_H

#include <QList>
#include "pdfpala.h"

#include <QPdfSelection>

namespace Tuonti {


class PdfRivi
{
public:
    PdfRivi();
    PdfRivi(const QPdfSelection& selection);
    ~PdfRivi();

    int vertaa(const QPdfSelection& selection) const;
    void tuo(const QPdfSelection& selection);

    void yhdistaPalat();

    PdfPala* pala() { return ekapala_;}

    QString teksti() const;

protected:
    PdfPala* ekapala_ = nullptr;

};

} // namespace Tuonti

#endif // TUONTI_PDFRIVI_H
