#include "tilinavausview.h"

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include "kirjaus/eurodelegaatti.h"
#include "tilinavausmodel.h"

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


QRegularExpression TilinAvausView::tiliRE__ = QRegularExpression(R"(\D*(\d{3,8})\W*(.+))", QRegularExpression::UseUnicodePropertiesOption);

