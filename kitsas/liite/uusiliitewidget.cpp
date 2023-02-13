#include "uusiliitewidget.h"

#include "ui_tositewg.h"
#include <QFileDialog>
#include "liitteetmodel.h"

UusiLiiteWidget::UusiLiiteWidget(QWidget *parent)
    : QWidget{parent},
      ui(new Ui::TositeWg)
{
    ui->setupUi(this);

    connect( ui->valitseTiedostoNappi, &QPushButton::clicked, this, &UusiLiiteWidget::valitseTiedosto);
}

UusiLiiteWidget::~UusiLiiteWidget()
{
    delete ui;
}

void UusiLiiteWidget::setModel(LiitteetModel *model)
{
    model_ = model;
}

void UusiLiiteWidget::valitseTiedosto()
{
    QString polku = QFileDialog::getOpenFileName(this, tr("Valitse tosite"),QString(),tr("Pdf-tiedostot (*.pdf);;Kuvat (*.png *.jpg);;Csv-tiedosto (*.csv);;Kaikki tiedostot (*)"));
    if( !polku.isEmpty())
    {
        model_->lisaaHetiTiedosto(polku);
    }
}
