#ifndef TUONTI_PDFSIVU_H
#define TUONTI_PDFSIVU_H

#include "pdfrivi.h"

#include <QList>
#include <QSize>
#include <QPdfSelection>

class QPdfDocument;

namespace Tuonti {


class PdfSivu
{
public:
    PdfSivu();
    ~PdfSivu();

    void tuo(QPdfDocument* doc, int sivu);

    QString teksti() const;

private:
    void lisaa(const QPdfSelection& selection);

protected:
    QList<PdfRivi*> rivit_;
    QSize sivunKoko_;
};

} // namespace Tuonti

#endif // TUONTI_PDFSIVU_H
