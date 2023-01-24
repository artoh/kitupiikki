#ifndef NAYTIN_PDFNAYTIN_H
#define NAYTIN_PDFNAYTIN_H

#include "abstraktinaytin.h"

class QPdfView;
class QPdfDocument;
class QBuffer;

namespace Naytin {

class PdfNaytin : public Naytin::AbstraktiNaytin
{
    Q_OBJECT
public:
    explicit PdfNaytin(const QByteArray& ba, QObject *parent = nullptr);

    QWidget *widget() override;
    QString tiedostonMuoto() const override { return tr("Pdf-tiedosto (*.pdf)");}
    QString tiedostonPaate() const override { return "pdf"; }

    QByteArray data() const override;

    bool voikoZoomata() const override { return true; }


public slots:
    void paivita() const override;
    void tulosta(QPrinter* printer) const override;

    virtual void zoomIn() override;
    virtual void zoomOut() override;
    virtual void zoomFit() override;


protected:
    QPdfDocument *doc_;
    QByteArray data_;
    QBuffer *buff_;
    QPdfView *view_;
    qreal skaala_;
};

} // namespace Naytin

#endif // NAYTIN_PDFNAYTIN_H
