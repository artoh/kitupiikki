#ifndef PDFLIITEVIEW_H
#define PDFLIITEVIEW_H

#include <QPdfView>
#include <QPrinter>

class PdfLiiteView : public QPdfView                
{
    Q_OBJECT
public:
    PdfLiiteView(QWidget* parent = nullptr);

    void tulosta(QPrinter *printer);
};

#endif // PDFLIITEVIEW_H
