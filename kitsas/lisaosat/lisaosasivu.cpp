#include "lisaosasivu.h"
#include "lisaosalistmodel.h"
#include "db/kirjanpito.h"
#include "lisaosat/lisaosalokiikkuna.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/avattupilvi.h"
#include "lisaosasortproxymodel.h"
#include "qwebengineprofile.h"
#include "yksityinenlisaosadialogi.h"
#include "kieli/kielet.h"

#include <QWebEngineView>
#include <QListView>
#include <QLabel>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopServices>
#include <QApplication>
#include <QWebEngineDownloadRequest>
#include <QSlider>
#include <QSettings>
#include <QSplitter>
#include <QWidget>
#include <QWebEngineSettings>

LisaosaSivu::LisaosaSivu(QWidget* parent) :
    KitupiikkiSivu(parent),
    listModel_(new LisaosaListModel(this))
{
    initUi();

    connect( listView_->selectionModel(), &QItemSelectionModel::currentChanged, this, &LisaosaSivu::valittu);
    connect( webView_, &QWebEngineView::iconChanged, this, &LisaosaSivu::kuvakeVaihtui);
    connect( listModel_, &LisaosaListModel::modelReset, this, &LisaosaSivu::listReseted);

    webView_->load(QUrl("qrc:/loading.html"));            

    connect( webView_->page(), &QWebEnginePage::newWindowRequested, this, &LisaosaSivu::openLinkInNewWindow);
    connect( webView_->page()->profile(), &QWebEngineProfile::downloadRequested, this, &LisaosaSivu::downloadRequested);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    connect( webView_->page(), &QWebEnginePage::permissionRequested, this, &LisaosaSivu::permissionRequest);
#endif

}


void LisaosaSivu::siirrySivulle()
{
    listModel_->load();
    if( splitter_->sizes().value(0) < 20 ) {
        initSlider();
    }
}

void LisaosaSivu::initSlider()
{
const int width = splitter_->width();
        int firstSize = width / 4;
        if( firstSize > 200) firstSize = 200;
        splitter_->setSizes(QList<int>() << firstSize << width - firstSize);
}


void LisaosaSivu::initUi()
{
    splitter_ = new QSplitter(Qt::Horizontal, this);

    webView_ = new QWebEngineView(this);
    listView_ = new QListView(this);
    logoLabel_ = new QLabel(this);
    nameLabel_ = new QLabel(this);
    nameLabel_->setFont(QFont("FreeSans", 14, QFont::Bold));
    
    zoomSlider_ = new QSlider(Qt::Horizontal, this);
    zoomLabel_ = new QLabel(tr("Zoom"), this);
    
    zoomSlider_->setRange(50, 200);
    zoomSlider_->setTickInterval(25);
    zoomSlider_->setTickPosition(QSlider::TicksBothSides);
    const int zoom = kp()->settings()->value("lisaosat/zoom", 100).toInt();
    
    connect( zoomSlider_, &QSlider::valueChanged, this, &LisaosaSivu::zoomChanged);
    zoomSlider_->setValue( zoom );    


    aktivoiNappi_ = new QPushButton(QIcon(":/pic/lisaa.png"), tr("Ota käyttöön"), this);
    passivoiNappi_ = new QPushButton(QIcon(":/pic/poista.png"), tr("Poista käytöstä"), this);

    kotiNappi_ = new QPushButton(QIcon(":/pic/talo.png"), tr("Alkuun"), this);
    connect( kotiNappi_, &QPushButton::clicked, this, &LisaosaSivu::valittu);

    lokiAktio_ = new QAction(QIcon(":/pic/Paivakirja64.png"), tr("Näytä tapahtumat"), this);
    yksityinenAktio_ = new QAction(QIcon(":/pic/lisaa.png"), tr("Ota käyttöön yksityinen lisäosa"), this);

    QMenu* toimintoMenu = new QMenu(this);
    toimintoMenu->addAction(lokiAktio_);
    toimintoMenu->addAction(yksityinenAktio_);

    toiminnotNappi_ = new QPushButton(QIcon(":/pic/Menu-Circles.png"), "", this);
    toiminnotNappi_->setMenu(toimintoMenu);

    QHBoxLayout* ylaLeiska = new QHBoxLayout;
    ylaLeiska->addWidget(logoLabel_);
    ylaLeiska->addWidget(nameLabel_);
    ylaLeiska->addStretch();
    ylaLeiska->addWidget(zoomSlider_);
    ylaLeiska->addWidget(zoomLabel_);
    ylaLeiska->addStretch();
    ylaLeiska->addWidget(kotiNappi_);
    ylaLeiska->addWidget(aktivoiNappi_);
    ylaLeiska->addWidget(passivoiNappi_);
    ylaLeiska->addWidget(toiminnotNappi_);

    QVBoxLayout* oikeaLeiska = new QVBoxLayout;
    oikeaLeiska->addLayout(ylaLeiska,0);
    oikeaLeiska->addWidget(webView_,1);
    QWidget* oikeaWidget = new QWidget(this);
    oikeaWidget->setLayout(oikeaLeiska);

    splitter_->addWidget(listView_);
    splitter_->addWidget(oikeaWidget);

    QHBoxLayout* leiska = new QHBoxLayout;
    leiska->addWidget(splitter_);
    setLayout(leiska);
    initSlider();
    
    LisaosaSortProxyModel* proxy = new LisaosaSortProxyModel(this);
    proxy->setSourceModel(listModel_);
    listView_->setModel(proxy);

    connect( aktivoiNappi_, &QPushButton::clicked, this, &LisaosaSivu::aktivoi);
    connect( passivoiNappi_, &QPushButton::clicked, this, &LisaosaSivu::passivoi);
    connect( lokiAktio_, &QAction::triggered, this, &LisaosaSivu::naytaLoki);
    connect( yksityinenAktio_, &QAction::triggered, this, &LisaosaSivu::yksityinen);
}

void LisaosaSivu::valittu()
{
    const QModelIndex& indeksi = listView_->currentIndex();
    if( indeksi.isValid()) {
        currentId_ = indeksi.data(LisaosaListModel::IdRooli).toString();
        hae(currentId_);
        const bool aktiivinen = indeksi.data(LisaosaListModel::AktiivinenRooli).toBool();
        const bool jarjestelmaTasolla = indeksi.data(LisaosaListModel::SystemRooli).toBool();
        aktivoiNappi_->setVisible( !aktiivinen );
        passivoiNappi_->setVisible( aktiivinen && !jarjestelmaTasolla);
        kotiNappi_->setVisible(aktiivinen);
    }
}

void LisaosaSivu::aktivoi()
{
    if( listView_->currentIndex().isValid()) {
        hae( listView_->currentIndex().data(LisaosaListModel::IdRooli).toString(), AKTIVOI);
    }
}

void LisaosaSivu::yksityinen()
{
    const QString id = YksityinenLisaosaDialogi::getId(this);
    if( !id.isEmpty()) {
        hae(id, AKTIVOI);
    }
}

void LisaosaSivu::naytaLoki()
{
    if( currentId_.isEmpty()) return;
    LisaosaLokiIkkuna* ikkuna = new LisaosaLokiIkkuna(this);
    ikkuna->show();
    ikkuna->lataaLoki(currentId_, nameLabel_->text());
}

void LisaosaSivu::passivoi()
{
    if( listView_->currentIndex().isValid()) {

        const QString nimi = listView_->currentIndex().data(LisaosaListModel::NimiRooli).toString();

        if (nimi.contains("Kitsas Pro Palkat")) {

            QMessageBox viestiloota;
            viestiloota.setText(tr("Kitsas Pro Palkat -lisäosan poistaminen käytöstä ei riitä katkaisemaan laskutusta lisäosasta tämän kirjanpidon osalta. Otathan lisäksi yhteyttä käyttäjätukeen kitsas.helpdesk@taopa.fi"));
            viestiloota.exec();
        }

        if (nimi.contains("Kitsas Pro Maksut")) {

            QMessageBox viestiloota;
            viestiloota.setText(tr("Kitsas Pro Maksut -lisäosan poistaminen käytöstä ei riitä katkaisemaan laskutusta lisäosasta tämän kirjanpidon osalta. Otathan lisäksi yhteyttä käyttäjätukeen kitsas.helpdesk@taopa.fi"));
            viestiloota.exec();
        }

        const QString addonId = listView_->currentIndex().data(LisaosaListModel::IdRooli).toString();

        KpKysely* kysely = kp()->pilvi()->loginKysely("/v1/permissions", KpKysely::PATCH);
        QVariantMap payload;
        payload.insert("owner", addonId);
        payload.insert("target", kp()->pilvi()->pilvi().bookId());
        payload.insert("rights", QStringList());
        payload.insert("roles", QStringList());
        connect( kysely, &KpKysely::vastaus, listModel_, &LisaosaListModel::load);
        kysely->kysy(QVariantList() << payload);
    }
}

void LisaosaSivu::hae(const QString lisaosaId, Toiminto toiminto)
{
    webView_->load(QUrl("qrc:/loading.html"));
    KpKysely* kysely = kp()->pilvi()->loginKysely("/v1/addons/" + lisaosaId);
    kysely->lisaaAttribuutti("target", kp()->pilvi()->pilvi().bookId());
    if(toiminto == AKTIVOI) {
        connect( kysely, &KpKysely::vastaus, this, &LisaosaSivu::aktivointiInfoSaapuu);
        connect(kysely, &KpKysely::virhe, this, [this]
                { QMessageBox::critical(this, tr("Lisäosan käyttöönottaminen epäonnistui"), tr("Lisäosan tunniste on virheellinen tai käyttöönotossa tapahtui virhe."));});
    } else {
        connect( kysely, &KpKysely::vastaus, this, &LisaosaSivu::infoSaapuu);
    }
    kysely->kysy();
}

void LisaosaSivu::infoSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    const QString url = map.value("url").toString();
    const QString callId = map.value("callId").toString();
    Monikielinen nimi(map.value("name"));

    nameLabel_->setText(nimi.teksti());

    const QString link = url + "?callId=" + callId + "&lang=" + Kielet::instanssi()->uiKieli() ;

    if( qApp->property("noweb").toBool()) {
        QDesktopServices::openUrl(link);
    } else {
        webView_->load(link);
    }
}

void LisaosaSivu::aktivointiInfoSaapuu(QVariant *data)
{
    QVariantMap map = data->toMap();
    const QStringList rights = map.value("rights").toStringList();
    const QString addonId = map.value("id").toString();

    KpKysely* kysely = kp()->pilvi()->loginKysely("/v1/permissions", KpKysely::PATCH);
    QVariantMap payload;
    payload.insert("owner", addonId);
    payload.insert("target", kp()->pilvi()->pilvi().bookId());
    payload.insert("rights", rights);
    payload.insert("roles", QStringList());
    connect( kysely, &KpKysely::vastaus, listModel_, &LisaosaListModel::load);

    kysely->kysy(QVariantList() << payload);
}

void LisaosaSivu::kuvakeVaihtui(const QIcon &icon)
{
    logoLabel_->setPixmap(icon.pixmap(32,32));
}

void LisaosaSivu::listReseted()
{
    for(int i = 0; i < listModel_->rowCount(); ++i) {
        const QModelIndex indeksi = listModel_->index(i,0);
        const QString id = indeksi.data(LisaosaListModel::IdRooli).toString();
        if(id == currentId_) {
            listView_->setCurrentIndex(indeksi);
            break;
        }
    }
    if( !listView_->currentIndex().isValid()) {
        listView_->setCurrentIndex( listView_->model()->index(0,0) );
    }
}

void LisaosaSivu::openLinkInNewWindow(QWebEngineNewWindowRequest &request)
{
    if( request.destination() == QWebEngineNewWindowRequest::InNewWindow || request.destination() == QWebEngineNewWindowRequest::InNewTab)
        QDesktopServices::openUrl(request.requestedUrl());
}

void LisaosaSivu::downloadRequested(QWebEngineDownloadRequest *download)
{
    connect( download, &QWebEngineDownloadRequest::isFinishedChanged, this, &LisaosaSivu::downloadFinished);
    download->accept();
}

void LisaosaSivu::downloadFinished()
{
    const QWebEngineDownloadRequest* download = qobject_cast<QWebEngineDownloadRequest*>(sender());
    if( download->isFinished()) {
        emit kp()->onni( tr("%1 ladattu").arg(download->downloadFileName()), Kirjanpito::Onni::Ladattu );
    }
}

void LisaosaSivu::zoomChanged(int value)
{
    webView_->setZoomFactor(value / 100.0);
    zoomLabel_->setText(QString("%1 %").arg(value));
    kp()->settings()->setValue("lisaosat/zoom", value);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)

void LisaosaSivu::permissionRequest(QWebEnginePermission permission)
{
    if( permission.permissionType() == QWebEnginePermission::PermissionType::ClipboardReadWrite) {
        permission.grant();
    } else {
        permission.deny();
    }
}

#endif
