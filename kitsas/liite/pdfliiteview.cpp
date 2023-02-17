#include "pdfliiteview.h"

#include <QPrintDialog>
#include <QPdfDocument>
#include <QPainter>

PdfLiiteView::PdfLiiteView(QWidget *parent)
    : QPdfView(parent)
{

}

void PdfLiiteView::tulosta(QPrinter *printer)
{
    QPrintDialog dlg(printer, this);
    dlg.setMinMax(1, document()->pageCount() + 1);


    if( dlg.exec() ) {
        QPainter painter(printer);
        QSizeF kohde(painter.window().width(), painter.window().height());

        int sivulta = 0;
        int sivulle = document()->pageCount();

        if( dlg.printRange() == QPrintDialog::PrintRange::PageRange ) {
            sivulta = dlg.fromPage() - 1;
            sivulle = dlg.toPage() - 1;
        }

        for(int i=sivulta; i < sivulle; i++) {

            if( i > sivulta)
                printer->newPage();

            QSizeF koko = document()->pagePointSize(i);
            QPdfDocumentRenderOptions options;
            if( koko.width() > koko.height())
                options.setRotation(QPdfDocumentRenderOptions::Rotation::Clockwise90);
            QImage image = document()->render(i, kohde.toSize(), options);
            painter.drawImage(0,0,image);
        }
    }
}
