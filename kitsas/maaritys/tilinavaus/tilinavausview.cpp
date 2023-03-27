#include "tilinavausview.h"

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include "kirjaus/eurodelegaatti.h"
#include "tilinavausmodel.h"

#include "tuonti/csvtuonti.h"
#include "tuonti/tilimuuntomodel.h"

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

QDate TilinAvausView::kkPaivaksi(const QString teksti)
{
    if( teksti.contains(" - ")) {
        QStringList osat = teksti.split(" - ");
        if( osat.count() == 2) {
            QDate loppu = QDate::fromString(osat.last(),"dd.MM.yyyy");
            if(loppu.isValid()) return loppu;
        }
    }

    QStringList patkina = teksti.split("/");
    if( patkina.length() == 2) {
        int kk = patkina.at(0).toInt();
        int vvvv = patkina.at(1).toInt();
        if( kk > 0 && kk < 13 && vvvv > 2010 && vvvv < 2200) {
            QDate eka(vvvv, kk, 1);
            return QDate(vvvv, kk, eka.daysInMonth());
        }
    }
    return QDate();
}

void TilinAvausView::tuoAvausTiedosto(const QString &polku)
{
    QFile file(polku);
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    QByteArray data = file.readAll();
    QList<QStringList> csv = Tuonti::CsvTuonti::csvListana(data);

    TiliMuuntoModel muunto(this);    

    const QDate avausPvm = kp()->asetukset()->pvm(AsetusModel::TilinAvausPvm);
    const Tilikausi kausi = kp()->tilikausiPaivalle(avausPvm);

    QList<QDate> paivat;
    paivat << kausi.paattyy();

    for(const QStringList& rivi : qAsConst(csv)) {
        if( rivi.length() < 2) continue;

        if( rivi.length() > 2 && kkPaivaksi(rivi.at(1)).isValid()) {
            QList<QDate>  haettuPaivaLista;
            for(int i=1; i < rivi.length(); i++) {
                QDate paiva = kkPaivaksi(rivi.at(i));
                if( !paiva.isValid())
                    break;
                if( paiva < kausi.alkaa() || paiva > kausi.paattyy()) {
                    haettuPaivaLista.clear();
                    break;
                }
                haettuPaivaLista << paiva;
            }
            if( haettuPaivaLista.length() > paivat.length()) {
                paivat = haettuPaivaLista;
            }
            continue;
        }

        bool saldoja = false;
        QList<Euro> saldot;

        for(int i=1; i < rivi.length() && i <= paivat.length(); i++) {
            Euro saldo = Euro::fromString(rivi.at(i));
            if( saldo ) saldoja = true;
            saldot << saldo;
        }

        const QString tilistr = rivi.value(0);        

        if( !saldoja ) continue;
        QRegularExpressionMatch mats = tiliRE__.match(tilistr);
        if( mats.hasMatch()) {
            const int tili = mats.captured(1).toInt();
            const QString nimi = mats.captured(2);

            muunto.lisaa(tili, nimi, saldot);
        }
    }

    if( muunto.rowCount() < 3) {
        return;
    }

    muunto.asetaSaldoPaivat(paivat);
    if( muunto.naytaMuuntoDialogi(this) == QDialog::Accepted) {
        model_->tuo(&muunto);
    }

}

QRegularExpression TilinAvausView::tiliRE__ = QRegularExpression(R"(\D*(\d{3,8})\W*(.+))", QRegularExpression::UseUnicodePropertiesOption);

