#include "tilinavausview.h"

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include "kirjaus/eurodelegaatti.h"
#include "tilinavausmodel.h"

#include "tuonti/csvtuonti.h"
#include "tuonti/tilimuuntomodel.h"
#include "tuonti/procountorsaldotuonti.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>

TilinAvausView::TilinAvausView(QWidget *parent) :
    QTableView{parent},
    model_(new TilinavausModel(this)),
    proxy_(new QSortFilterProxyModel{this}),
    suodatus_(new QSortFilterProxyModel{this})
{

    proxy_->setSourceModel(model_);
    proxy_->setFilterRole(TilinavausModel::KaytossaRooli);
    proxy_->setFilterFixedString("1");
    proxy_->setSortRole(TilinavausModel::LajitteluRooli);

    suodatus_->setSourceModel(proxy_);
    suodatus_->setFilterCaseSensitivity(Qt::CaseInsensitive);

    setModel(suodatus_);

    setItemDelegateForColumn( TilinavausModel::SALDO, new EuroDelegaatti);
    horizontalHeader()->setSectionResizeMode(TilinavausModel::NIMI, QHeaderView::Stretch);

    setAcceptDrops(true);
}

TilinavausModel *TilinAvausView::avausModel()
{
    return model_;
}

void TilinAvausView::naytaPiilotetut(bool naytetaanko)
{
    if( naytetaanko) {
        proxy_->setFilterFixedString("0");
    } else
        proxy_->setFilterFixedString("1");
}

void TilinAvausView::naytaVainKirjaukset(bool naytetaanko)
{
    if( naytetaanko ) {
        proxy_->setFilterFixedString("2");
    } else {
        proxy_->setFilterFixedString("1");
    }
}

void TilinAvausView::suodata(const QString &suodatusteksti)
{
    if( suodatusteksti.toInt())
        suodatus_->setFilterRole(TilinavausModel::NumeroRooli);
    else
        suodatus_->setFilterRole(TilinavausModel::NimiRooli);
    suodatus_->setFilterFixedString(suodatusteksti);

}

void TilinAvausView::nollaa()
{
    model_->lataa();
    proxy_->sort(0);
}

void TilinAvausView::dragEnterEvent(QDragEnterEvent *event)
{
    if( event->mimeData()->hasUrls() )
    {
        for( const QUrl& url: event->mimeData()->urls())
        {
            if(url.isLocalFile())
                event->accept();
        }
    }
}

void TilinAvausView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void TilinAvausView::dropEvent(QDropEvent *event)
{
    if( event->mimeData()->hasUrls())
    {
        QList<QUrl> urlit = event->mimeData()->urls();
        foreach (QUrl url, urlit)
        {
            if( url.isLocalFile())
            {
                tuoAvausTiedosto(url.toLocalFile());
            }
        }
    }

}

void TilinAvausView::tuoAvausTiedosto(const QString &polku)
{
    QFile file(polku);
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QByteArray data = file.readAll();

    TiliMuuntoModel muunto(this);    

    const QDate avausPvm = kp()->asetukset()->pvm(AsetusModel::TilinAvausPvm);
    const Tilikausi kausi = kp()->tilikausiPaivalle(avausPvm);

    if( !ProcountorSaldoTuonti::alustaMuunto(data, muunto, kausi).isValid())
        return;

    if( muunto.naytaMuuntoDialogi(this) == QDialog::Accepted) {
        model_->tuo(&muunto);
    }

}

QRegularExpression TilinAvausView::tiliRE__ = QRegularExpression(R"(\D*(\d{3,8})\W*(.+))", QRegularExpression::UseUnicodePropertiesOption);

