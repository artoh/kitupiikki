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
    int etsi(const QStringList& tekstit, int maxRivi = 0);
    QList<PdfPala*> etsiPalat(const QStringList& tekstit, const QRect alue = QRect());
    QList<PdfPala*> etsiPalat(const QRect alue);

    PdfPala* etsiPala(const QString& teksti, const QRect& alue);

    int riveja() const;
    PdfRivi* rivi(int rivi);

private:
    void lisaa(PdfPala* uusipala);

protected:
    QList<PdfRivi*> rivit_;
    QSize sivunKoko_;

};

} // namespace Tuonti

#endif // TUONTI_PDFSIVU_H
