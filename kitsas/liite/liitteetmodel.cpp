#include "liitteetmodel.h"
#include "db/kirjanpito.h"
#include "liite/cacheliite.h"
#include "liite/liitecache.h"

#include "model/tosite.h"
#include "pilvi/pilvimodel.h"
#include "tuonti/csvtuonti.h"

#include "tuonti/pdf/pdftiedosto.h"
#include "tuonti/pdf/tuontiinfo.h"

#include "db/tositetyyppimodel.h"

#include "tuonti/csvtuonti.h"
#include "tuonti/titotuonti.h"
#include "tuonti/tesseracttuonti.h"
#include "tuonti/palkkafituonti.h"
#include "pilvi/pilvikysely.h"

#include <QBuffer>
#include <QImage>
#include <QVector>
#include <QByteArray>
#include <QFile>
#include <QMessageBox>
#include <QPdfDocument>
#include <QSettings>
#include <QMimeData>
#include <QUrl>
#include <QTextDocument>
#include <QPdfWriter>

#include <QTimer>

LiitteetModel::LiitteetModel(QObject *parent)
    : QAbstractListModel(parent),
      puskuri_{new QBuffer()},
      pdfDoc_{new QPdfDocument(this)}
{
    connect( kp()->liiteCache(), &LiiteCache::liiteHaettu,
             this, &LiitteetModel::valimuistiLiite);
    connect( kp()->liiteCache(), &LiiteCache::hakuVirhe,
             this, &LiitteetModel::hakuVirhe);
    connect( pdfDoc_, &QPdfDocument::statusChanged,
             this, &LiitteetModel::pdfTilaVaihtui);

}

LiitteetModel::~LiitteetModel()
{
    for(auto ptr: liitteet_)
        delete ptr;
    liitteet_.clear();
}

int LiitteetModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return liitteet_.count();
}

QVariant LiitteetModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    const Liite* liite = liitteet_.at(index.row());

    switch( role ) {
        case Qt::DisplayRole:
            if(liite->nimi().isEmpty())
                return liite->rooli();
            else
                return liite->nimi();
        case Qt::DecorationRole:
            if( liite->thumb().isNull() )
                return QIcon(":/pic/text64.png");
            else
                return liite->thumb();
        case SisaltoRooli:
            return *liite->dataPtr();
        case NimiRooli:
            return liite->nimi();
        case TyyppiRooli:
            return liite->tyyppi();
        case RooliRooli:
            return liite->rooli();
        case IdRooli:
            return liite->id();
    }

    return QVariant();
}

Qt::ItemFlags LiitteetModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;
}

void LiitteetModel::lataa(const QVariantList &data)
{
    beginResetModel();

    liitteet_.clear();
    naytettavaIndeksi_ = -1;

    for(const auto& item : qAsConst(data)) {
        const QVariantMap& map = item.toMap();
        liitteet_.append(new Liite(this, map));
    }

    endResetModel();

    // TODO: Parhaimman näytettävän tunnistus
    if( interaktiivinen_) {
        if(liitteet_.isEmpty()) {
            nayta(-1);
        } else {
            nayta(0);
        }
    }

    tarkastaKaikkiLiitteet();
}

void LiitteetModel::clear()
{
    beginResetModel();
    for(auto item: liitteet_)
        delete item;
    liitteet_.clear();
    endResetModel();
    nayta(-1);
}

void LiitteetModel::asetaInteraktiiviseksi(bool onko)
{
    interaktiivinen_ = onko;
}

bool LiitteetModel::lisaa(QByteArray liite, const QString &tiedostonnimi, const QString &rooli)
{
    QByteArray sisalto = esikasittely(liite, tiedostonnimi);
    if( sisalto.isNull())
        return false;

    // Tarkastus ja esikäsittely

    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    Liite* uusiLiite = new Liite(this, sisalto, tiedostonnimi, rooli);
    liitteet_.append( uusiLiite);
    endInsertRows();

    nayta( liitteet_.count() - 1 );

    return true;
}

bool LiitteetModel::lisaaHeti(QByteArray liite, const QString &polku)
{
//    qDebug() << "Lisays " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    // Tarkastus ja esikäsittely
    QFileInfo info(polku);
    QByteArray sisalto = esikasittely(liite, info.fileName());
    if( sisalto.isNull())
        return false;

    Liite* uusiLiite = new Liite(this, sisalto, polku);
    if( uusiLiite->tyyppi() == "application/octet-stream") {
        if(QMessageBox::question(nullptr, tr("Liitetiedoston tyyppiä ei tueta"),
                              tr("Tätä liitetiedostoa ei voi välttämättä näyttää Kitsaalla eikä sisällyttää arkistoon.\n"
                                 "Haluatko silti lisätä tämän tiedoston?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) != QMessageBox::Yes)
        delete uusiLiite;
        return false;
    }

    // Tallennus
    beginInsertRows(QModelIndex(), rowCount(), rowCount());    
    liitteet_.append( uusiLiite);
    endInsertRows();

    Tosite* tosite = qobject_cast<Tosite*>(parent());
    Q_ASSERT(tosite);

    bool tuoTiedot = liitteet_.count() == 1 && !tosite->tilioterivi();
    bool tallennusOcr = tuoTiedot && uusiLiite->tyyppi() == "image/jpeg" &&
          kp()->settings()->value("OCR").toBool() && qobject_cast<PilviModel*>(kp()->yhteysModel());

    uusiLiite->liita(tallennusOcr);

    // Tuonti
    if( tuoTiedot && !tallennusOcr) {
        if( uusiLiite->tyyppi() == "application/pdf") {
            // Tehdään pdf-tuonti heti kun on ladattuna
            pdfTuontiIndeksi_ = liitteet_.count() - 1;
        } else {
            tuoLiite(uusiLiite->tyyppi(), liite);
        }
    }

    nayta( liitteet_.count() - 1 );
    return true;

}

bool LiitteetModel::lisaaHetiTiedosto(const QString &polku)
{
    QByteArray ba;
    QFile tiedosto(polku);
    if( !tiedosto.open(QIODevice::ReadOnly) )
    {
        QMessageBox::critical(nullptr, tr("Tiedostovirhe"),
                              tr("Tiedoston %1 avaaminen epäonnistui \n%2").arg(polku, tiedosto.errorString()));
        return false;
    }

    ba = tiedosto.readAll();
    tiedosto.close();
    return lisaaHeti( ba, polku );
}

int LiitteetModel::tallennettaviaLiitteita() const
{
    int lkm = 0;
    for( const auto ptr : liitteet_) {
        if( ptr->tila() == Liite::TALLENNETTAVA) {
            lkm++;
        }
    }
    return lkm;
}

void LiitteetModel::tallennaLiitteet(int tositeId)
{
    if( !tallennettaviaLiitteita()) {
        emit liitteetTallennettu();
    } else {
        for(auto ptr : liitteet_) {
            if( ptr->tila() == Liite::TALLENNETTAVA) {
                ptr->tallenna(tositeId);
            }
        }
    }
}

void LiitteetModel::poistaInboxistaLisattyjenTiedostot()
{
    const QString inbox = kp()->settings()->value( kp()->asetukset()->uid() + "/KirjattavienKansio" ).toString();
    if( inbox.isEmpty()) return;

    const bool siirto = kp()->settings()->value( kp()->asetukset()->uid() + "/KirjattavienSiirto" ).toBool();
    const QString siirtoKansio = siirto ? kp()->settings()->value( kp()->asetukset()->uid() + "/KirjattavienSiirtoKansio" ).toString() : QString();

    for(auto ptr: liitteet_) {
        if( ptr->tila() == Liite::LIITETTY && ptr->polku().startsWith(inbox)) {
            ptr->poistaInboxistaLisattyTiedosto(siirtoKansio);
        }
    }
}

void LiitteetModel::nayta(int indeksi)
{
    naytettavaIndeksi_ = indeksi;
    puskuri_->close();

    emit valittuVaihtui(indeksi);

    if( indeksi > -1) {
        if( liitteet_.at(indeksi)->kaytettavissa()) {
            naytaKayttajalle();
        }
    }
}

QModelIndex LiitteetModel::naytettava() const
{
    if( naytettavaIndeksi_ < 0)
        return QModelIndex();
    else
        return index(naytettavaIndeksi_,0);
}

bool LiitteetModel::tallennetaanko() const
{
    for(const auto ptr : liitteet_) {
        if( ptr->tila() == Liite::LIITETAAN) return true;
    }
    return false;
}

QVariantList LiitteetModel::liitettavat() const
{
    QVariantList lista;
    for(const auto ptr: liitteet_) {
        if( ptr->tila() == Liite::LIITETTY) {
            lista.append(ptr->id());
        }
    }
    return lista;
}

void LiitteetModel::poista(int indeksi)
{

    beginRemoveRows(QModelIndex(),indeksi, indeksi);
    Liite* liite = liitteet_.takeAt(indeksi);
    endRemoveRows();

    liite->poista();
    delete liite;
}

QByteArray *LiitteetModel::sisalto()
{
    if( naytettavaIndeksi_ < 0)
        return nullptr;
    QByteArray* data = liitteet_.at(naytettavaIndeksi_)->dataPtr();
    return data;
}

void LiitteetModel::liitteenTilaVaihtui(Liite::LiiteTila uusiTila)
{
    if( uusiTila == Liite::TALLENNETTU && !tallennettaviaLiitteita()) {
        emit liitteetTallennettu();
    }

    if( tallennetaanko() != tallennetaanko_) {
        tallennetaanko_ = !tallennetaanko_;
        emit liitettaTallennetaan(tallennetaanko_);
    }
}

void LiitteetModel::ocr(const QVariantMap &data)
{
    emit tuonti(data);
}

bool LiitteetModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &/*parent*/) const
{
    if( action == Qt::IgnoreAction)
        return true;

    return data->hasUrls() || data->formats().contains("image/jpg") || data->formats().contains("image/png");
}

bool LiitteetModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex &/*parent*/)
{
    if( action == Qt::IgnoreAction)
        return true;

    int lisatty = 0;
    // Liitetiedosto pudotettu
    if( data->hasUrls())
    {
        QList<QUrl> urlit = data->urls();
        for(const auto& url : qAsConst( urlit ))
        {
            if( url.isLocalFile())
            {
                QFileInfo info( url.toLocalFile());
                lisaaHetiTiedosto(info.absoluteFilePath());
                lisatty++;
            }
        }
    }
    if( lisatty)
        return true;

    if( !lisatty && data->formats().contains("image/jpg"))
    {
        lisaaHeti( data->data("image/jpg"), tr("liite.jpg") );
        return true;
    }
    else if(!lisatty && data->formats().contains("image/png"))
    {
        lisaaHeti( data->data("image/png"), tr("liite.jpg") );
        return true;
    }

    return false;
}


void LiitteetModel::valimuistiLiite(int liiteId)
{
    for(int i=0; i < liitteet_.count(); i++) {
        if( liitteet_.at(i)->id() == liiteId) {
            emit dataChanged( createIndex(i,0), createIndex(i,0), QVector<int>() << Qt::DecorationRole );
            if( i == naytettavaIndeksi_ ) {
                naytaKayttajalle();
            }
        }
    }
    tarkastaKaikkiLiitteet();
}

void LiitteetModel::naytaKayttajalle()
{
    if(interaktiivinen_ && naytettavaIndeksi_ > -1 && naytettavaIndeksi_ < liitteet_.count()) {
        QByteArray* data = liitteet_.at(naytettavaIndeksi_)->dataPtr();
        if( !data) return;

        if( data->startsWith("%PDF")) {
//            qDebug() << "Lataus " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

            puskuri_->setBuffer(data);
            puskuri_->open(QIODevice::ReadOnly);
            pdfDoc_->load(puskuri_);
            emit naytaPdf();
        } else {
            emit naytaSisalto();
        }
    }
}

void LiitteetModel::pdfTilaVaihtui(QPdfDocument::Status status)
{
//    qDebug() << "Tila  " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    qApp->processEvents();  // Jotta saadaan latausnäkymä
    if( status == QPdfDocument::Status::Ready && naytettavaIndeksi_ == pdfTuontiIndeksi_) {
        Tuonti::PdfTiedosto pdfTuonti(pdfDoc_);
//        qDebug() << "Luettu " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        QVariantMap tuotu = pdfTuonti.tuo( kp()->tuontiInfo() );
//        qDebug() << "Tuotu: " << QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        pdfTuontiIndeksi_ = -1;
        if( tuotu.value("tyyppi").toInt() == TositeTyyppi::TILIOTE) {
            KpKysely *kysely = kpk("/tuontitulkki", KpKysely::POST);
            connect( kysely, &KpKysely::vastaus, this, [this] (QVariant* var) { emit this->tuonti(var->toMap()); });
            kysely->kysy(tuotu);
        } else {
            emit this->tuonti( tuotu);
        }
    }
}


void LiitteetModel::tuoLiite(const QString& tyyppi, const QByteArray& sisalto)
{

    if( tyyppi == "text/csv" && sisalto.startsWith("T;") ) {
        emit tuonti( PalkkaFiTuonti::tuo(sisalto) );
    } else if( sisalto.startsWith("T00322100") || tyyppi == "text/csv" ) {
        QVariant tuotu = sisalto.startsWith("T00322100") ?
                    Tuonti::TitoTuonti::tuo(sisalto)  // Konekielinen tiliote
                  : Tuonti::CsvTuonti::tuo(sisalto);
        KpKysely* kysely = kpk("/tuontitulkki", KpKysely::POST);
        connect( kysely, &KpKysely::vastaus, this, [this] (QVariant* var)
            { emit this->tuonti(var->toMap()); });
        kysely->kysy(tuotu);
    } else if( tyyppi == "image/jpeg" && kp()->settings()->value("OCR").toBool() &&
               !qobject_cast<PilviModel*>(kp()->yhteysModel()) &&
               kp()->pilvi()->tilausvoimassa()) {
        // Paikallinen kirjanpito, tekstintunnistus pilvessä
        emit ocrKaynnissa(true);
        Tuonti::TesserActTuonti *tesser = new Tuonti::TesserActTuonti(this);
        connect( tesser, &Tuonti::TesserActTuonti::tuotu, this,
                 [this] (const QVariantMap& data) { emit this->tuonti(data); emit ocrKaynnissa(false); });
        tesser->tuo(sisalto);
    }

    if(  kp()->pilvi()->tilausvoimassa() &&
            (sisalto.startsWith("<?xml version=\"1.0\" encoding=\"ISO-8859-15\"?>") ||
            (sisalto.startsWith("<SOAP-ENV:"))) &&
            sisalto.contains("<Finvoice")) {
        // Käsin lisätyn Finvoice-laskun sisällön liittäminen
        QMap<QString,QString> meta;
        meta.insert("Content-type","application/xml;charset=ISO-8859-15");

        PilviKysely* jsonk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                                              kp()->pilvi()->finvoiceOsoite() + "/tojson");
        connect( jsonk, &PilviKysely::vastaus, this,
                 [this] (QVariant* data)
                  { emit this->tuonti( data->toMap()); });
        jsonk->lahetaTiedosto(sisalto, meta);
    }
}

void LiitteetModel::tarkastaKaikkiLiitteet()
{
    for( const auto ptr : liitteet_) {
        if( ptr->tila() == Liite::HAETAAN) {
            return;
        }
    }
    emit kaikkiLiitteetHaettu();
}

QByteArray LiitteetModel::esikasittely(QByteArray sisalto, const QString& tiedostonnimi)
{
    // Muunnetaan kaikki kuvatiedostot jpg-kuviksi (ei kuiteskaan pdf)
    QImage image = sisalto.startsWith("%PDF") ? QImage() : image.fromData(sisalto);
    if( !image.isNull()) {
        int koko = kp()->settings()->value("KuvaKoko",2048).toInt();
        if( image.width() > image.height()) {
            if( image.width() > koko) {
                image = image.scaledToWidth(koko);
            }
        } else {
            if( image.height() > koko) {
                image = image.scaledToHeight(koko);
            }
        }
        if( kp()->settings()->value("KuvaMustavalko").toBool()) {
            image = image.convertToFormat(QImage::Format_Grayscale8);
        }
        QBuffer buffer(&sisalto);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer,"JPG", kp()->settings()->value("KuvaLaatu",40).toInt());
    } else if ( sisalto.left(128).contains(QByteArray("<html")) || sisalto.left(128).contains(QByteArray("<HTML")) ) {
        QTextDocument doc;
        doc.setHtml(Tuonti::CsvTuonti::haistettuKoodattu(sisalto));
        QByteArray array;
        QBuffer buffer(&array);
        buffer.open(QIODevice::WriteOnly);
        QPdfWriter writer(&buffer);
        writer.setTitle(tiedostonnimi);
        writer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
        writer.setPageSize(QPageSize(QPageSize::A4));

        doc.print(&writer);
        sisalto = array;

    }

    if( sisalto.length() > 10 * 1024 * 1024 ) {
        QMessageBox::critical(nullptr, tr("Liitetiedosto liian suuri"),
                              tr("Liitetiedostoa ei voi lisätä kirjanpitoon, koska liite on kooltaan liian suuri.\n"
                                 "Voit lisätä enintään 10 megatavun kokoisen liitteen."));
        return QByteArray();
    }  

    return sisalto;
}



