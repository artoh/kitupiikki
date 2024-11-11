#include "uusiliitewidget.h"

#include "ui_tositewg.h"
#include <QFileDialog>
#include "liitteetmodel.h"
#include "kirjaus/mallipohjamodel.h"

#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QClipboard>

UusiLiiteWidget::UusiLiiteWidget(QWidget *parent)
    : QWidget{parent},
      ui(new Ui::TositeWg)
{
    ui->setupUi(this);

    connect( ui->valitseTiedostoNappi, &QPushButton::clicked, this, &UusiLiiteWidget::valitseTiedosto);
    connect( qApp->clipboard(), &QClipboard::dataChanged, this, &UusiLiiteWidget::tarkistaLeikepoyta);

    ui->malliView->setModel(MallipohjaModel::instanssi());
    connect( MallipohjaModel::instanssi(), &MallipohjaModel::modelReset, this, &UusiLiiteWidget::nollaaPohjat);

    connect( ui->liitaNappi, &QPushButton::clicked, this, &UusiLiiteWidget::leikepoydalta);

    connect(ui->malliView, &QListView::clicked, this, [this] (const QModelIndex& index)
        {emit this->lataaPohja(index.data(Qt::UserRole).toInt());});

    tarkistaLeikepoyta();
    nollaaPohjat();
}

UusiLiiteWidget::~UusiLiiteWidget()
{
    delete ui;
}

void UusiLiiteWidget::setModel(LiitteetModel *model)
{
    model_ = model;
}

void UusiLiiteWidget::naytaPohja(bool naytetaanko)
{
    naytaPohja_ = naytetaanko;
    nollaaPohjat();
}

void UusiLiiteWidget::tarkistaLeikepoyta()
{
    QStringList formaatit = qApp->clipboard()->mimeData()->formats();

    ui->liitaNappi->setVisible( formaatit.contains("image/png") ||
                                formaatit.contains("image/jpg") );
}

void UusiLiiteWidget::valitseTiedosto()
{
    QStringList polku = QFileDialog::getOpenFileNames(this, tr("Valitse tosite"),QString(),tr("Pdf-tiedostot (*.pdf);;Kuvat (*.png *.jpg);;Csv-tiedosto (*.csv);;Kaikki tiedostot (*)"));
    for(const auto& p : polku)
    {
        model_->lisaaHetiTiedosto(p);
    }
}

void UusiLiiteWidget::nollaaPohjat()
{
    ui->pohjaGroup->setVisible( naytaPohja_ && ui->malliView->model()->rowCount() );
}

void UusiLiiteWidget::leikepoydalta()
{
    if( qApp->clipboard()->mimeData()->formats().contains("image/jpg"))
        model_->lisaaHeti(qApp->clipboard()->mimeData()->data("image/jpg"), tr("liite.jpg") );
    else if( qApp->clipboard()->mimeData()->formats().contains("image/png"))
        model_->lisaaHeti(qApp->clipboard()->mimeData()->data("image/png"), tr("liite.png") );

}
