/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "tositeliitteet.h"
#include "db/kirjanpito.h"


#include <QIcon>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>
#include <QBuffer>
#include <QDebug>
#include <QSettings>
#include <QPdfWriter>
#include <QImage>
#include <poppler/qt5/poppler-qt5.h>

#include "db/tositetyyppimodel.h"
#include "tuonti/pdftuonti.h"
#include "tuonti/csvtuonti.h"
#include "tuonti/titotuonti.h"
#include "tuonti/tesseracttuonti.h"
#include "tuonti/palkkafituonti.h"
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikysely.h"

TositeLiitteet::TositeLiitteet(QObject *parent)
    : QAbstractListModel(parent)
{
}



int TositeLiitteet::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return liitteet_.count();
}


QVariant TositeLiitteet::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TositeLiite liite = liitteet_.value(index.row());
    if( role == Qt::DisplayRole)
    {        

        if( liite.getNimi().isEmpty())
            return liite.getRooli();
        else
            return liite.getNimi();
    }
    else if( role == Qt::DecorationRole) {
        if( !liite.getThumb().isEmpty()) {
            QPixmap pixmap;
            pixmap.loadFromData(liite.getThumb(), "PNG");
            return QIcon(pixmap);
        } else if( liite.getRooli() == "lasku")
            return QIcon(":/pic/lasku.png");
        else if( liite.getNimi().endsWith(".pdf"))
            return QIcon(":/pic/pdf.png");
        else if( liite.getNimi().endsWith(".jpg"))
            return QIcon(":/pic/kuva2.png");
        return QIcon(":/pic/tekstisivu.png");
    } else if( role == SisaltoRooli) {
        return liite.getSisalto();
    } else if( role == NimiRooli) {
        return liite.getNimi();
    } else if( role == TyyppiRooli) {
        return KpKysely::tiedostotyyppi(liite.getSisalto());
    }

    // FIXME: Implement me!
    return QVariant();
}

Qt::ItemFlags TositeLiitteet::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index) | Qt::ItemIsDropEnabled;

}

void TositeLiitteet::lataa(QVariantList data)
{
    beginResetModel();
    liitteet_.clear();    
    tallennetaan_ = false;

    for( QVariant item : data) {
        QVariantMap map = item.toMap();

        liitteet_.append( TositeLiite( map.value("id").toInt(),
                                       map.value("nimi").toString(),
                                       QByteArray(),
                                       map.value("roolinimi").toString()) );
    }
    endResetModel();

    if( naytaLiite_ ) {
        emit naytaliite("*LADATAAN*");

        // Varmistetaan, että ensisijaisesti näytetään laskun kuva, ei
        // xml-laskua
        for(int i=0; i < liitteet_.count(); i++) {
            QString tyyppi = data.value(i).toMap().value("tyyppi").toString();
            if( tyyppi == "application/pdf" || tyyppi == "application/jpg")
                emit nayta(i);

                // Haetaan vielä esikatseltavat
                for(i++; i < liitteet_.count(); i++) {
                    QVariantMap map = data.value(i).toMap();
                    QString tyyppi = map.value("tyyppi").toString();
                    if( tyyppi == "application/pdf" || tyyppi == "application/jpg") {
                        KpKysely* kysely = kpk(QString("/liitteet/%1").arg( map.value("id").toInt()));
                        connect( kysely, &KpKysely::vastaus, [this, i] (QVariant* data) {this->liitesaapuuValmiiksi(data, i);});
                        kysely->kysy();
                }
                return;
            }
        }

        if( liitteet_.count())
            nayta(0);
        else
            emit naytaliite(QByteArray());
    }
}

void TositeLiitteet::clear()
{
    beginResetModel();
    liitteet_.clear();
    tallennetaan_ = false;
    inboxista_.clear();
    endResetModel();
    emit naytaliite(QByteArray());
}

void TositeLiitteet::tallennettu()
{
    if( !inboxista_.isEmpty()) {
        if( !kp()->settings()->value( kp()->asetus("UID") + "/KirjattavienSiirto" ).toBool() ) {
            for(QString tiedosto : inboxista_) {
                QFile::remove(tiedosto);
            }
        } else {
            QString minne = kp()->settings()->value( kp()->asetus("UID") + "/KirjattavienSiirtoKansio" ).toString();
            QDir kohde(minne);           
            for( QString tiedosto : inboxista_) {                
                QFileInfo info(tiedosto);
                // #809 Jos tiedosto on jo, nimetään se hiljaisesti uudelleen
                QString tnimi = info.fileName();
                for(int yritys=1; kohde.exists(tnimi) && yritys < 1000; yritys++) {
                    tnimi = QString("%1_%2.%3").arg(info.baseName()).arg(yritys).arg(info.completeSuffix());
                }
                if( QFile::copy( info.absoluteFilePath(), kohde.absoluteFilePath(tnimi) )) {
                    QFile::remove(info.absoluteFilePath());
                }
            }
        }
    }
}

bool TositeLiitteet::lisaa(const QByteArray &sisalto, const QString& tiedostonnimi, const QString& rooli)
{
    if( sisalto.isNull())
        return false;

    QString nimi = tiedostonnimi;

    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    liitteet_.append( TositeLiite(0, nimi, sisalto, rooli) );
    endInsertRows();

    return true;
}

bool TositeLiitteet::lisaaTiedosto(const QString &polku)
{
    return lisaa( lueTiedosto(polku), QFileInfo(polku).fileName());
}

bool TositeLiitteet::lisaaHeti(QByteArray liite, const QString &tiedostonnimi, const QString& polku)
{
    QString tnimi = tiedostonnimi;

    if( liite.isNull())
        return false;

    // Muunnetaan kaikki kuvatiedostot jpg-kuviksi
    QImage image = image.fromData(liite);
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
        QBuffer buffer(&liite);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer,"JPG", kp()->settings()->value("KuvaLaatu",40).toInt());
    } else if ( liite.left(128).contains(QByteArray("<html")) || liite.left(128).contains(QByteArray("<HTML")) ) {
        QTextDocument doc;
        doc.setHtml(Tuonti::CsvTuonti::haistettuKoodattu(liite));
        QByteArray array;
        QBuffer buffer(&array);
        buffer.open(QIODevice::WriteOnly);
        QPdfWriter writer(&buffer);
        writer.setTitle(tiedostonnimi);
        writer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
        writer.setPageSize(QPdfWriter::A4);

        doc.print(&writer);
        liite = array;
        tnimi.append(".pdf");

    }

    if( liite.length() > 10 * 1024 * 1024 ) {
        QMessageBox::critical(nullptr, tr("Liitetiedosto liian suuri"),
                              tr("Liitetiedostoa ei voi lisätä kirjanpitoon, koska liite on kooltaan liian suuri.\n"
                                 "Voit lisätä enintään 10 megatavun kokoisen liitteen."));
        return false;
    }


    QString tyyppi = KpKysely::tiedostotyyppi(liite);
    if(tyyppi == "application/octet-stream") {
        if(QMessageBox::question(nullptr, tr("Liitetiedoston tyyppiä ei tueta"),
                              tr("Tätä liitetiedostoa ei voi välttämättä näyttää Kitsaalla eikä sisällyttää arkistoon.\n"
                                 "Haluatko silti lisätä tämän tiedoston?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) != QMessageBox::Yes)
        return false;
    } else {
        emit naytaliite( liite );
    }

    beginInsertRows( QModelIndex(), liitteet_.count(), liitteet_.count() );
    liitteet_.append( TositeLiite(0, tnimi, liite, QString(), polku) );
    int liiteIndeksi = liitteet_.count() - 1;
    endInsertRows();

    tallennetaan_ = true;
    emit liitettaTallennetaan(true);

    KpKysely* liitekysely = kpk("/liitteet", KpKysely::POST);
    connect( liitekysely, &KpKysely::lisaysVastaus, [this, liiteIndeksi] (const QVariant& data, int id) {
            this->liiteLisatty(data, id, liiteIndeksi);
        });


    // Ensimmäisestä liitteestä tuodaan tiedot
    if( liitteet_.count() == 1) {        
        if( tyyppi == "application/pdf") {
            const QVariantMap &tuotu = Tuonti::PdfTuonti::tuo(liite);
            if( tuotu.value("tyyppi").toInt() == TositeTyyppi::TILIOTE) {
                KpKysely *kysely = kpk("/tuontitulkki", KpKysely::POST);
                connect( kysely, &KpKysely::vastaus, [this] (QVariant* var) { emit this->tuonti(var->toMap()); });
                kysely->kysy(tuotu);
            } else {
                emit this->tuonti( tuotu);
            }
        } else if( tyyppi == "text/csv" && liite.startsWith("T;")) {
            emit tuonti(PalkkaFiTuonti::tuo(liite));
        } else if(  liite.startsWith("T00322100") ||  tyyppi == "text/csv") {
            QVariant tuotu = liite.startsWith("T00322100") ?
                        Tuonti::TitoTuonti::tuo(liite) :
                        Tuonti::CsvTuonti::tuo(liite);
            KpKysely *kysely = kpk("/tuontitulkki", KpKysely::POST);
            connect( kysely, &KpKysely::vastaus, [this] (QVariant* var) { emit this->tuonti(var->toMap()); });
            kysely->kysy(tuotu);
        } else if( tyyppi == "image/jpeg" && kp()->settings()->value("OCR").toBool() ) {
            if( qobject_cast<PilviModel*>(kp()->yhteysModel()) ) {
                liitekysely->lisaaAttribuutti("ocr","json");
                connect(liitekysely, &KpKysely::vastaus, [this] (QVariant* data) { emit this->tuonti(data->toMap());});
            } else if( kp()->pilvi()->tilausvoimassa() ) {
                emit ocrKaynnissa(true);
                Tuonti::TesserActTuonti *tesser = new Tuonti::TesserActTuonti(this);
                connect( tesser, &Tuonti::TesserActTuonti::tuotu,
                         [this] (const QVariantMap& data) { emit this->tuonti(data); emit ocrKaynnissa(false); });
                tesser->tuo(liite);
            }
        }
    }

    QMap<QString,QString> meta;
    meta.insert("Filename", tnimi);
    meta.insert("Content-type", tyyppi);
    liitekysely->lahetaTiedosto(liite, meta);

    if(  kp()->pilvi()->tilausvoimassa() &&
            (liite.startsWith("<?xml version=\"1.0\" encoding=\"ISO-8859-15\"?>") ||
            (liite.startsWith("<SOAP-ENV:"))) &&
            liite.contains("<Finvoice")) {
        liitaFinvoice(liite);
    }

    return true;

}

bool TositeLiitteet::lisaaHetiTiedosto(const QString &polku)
{
    QString inbox = kp()->settings()->value( kp()->asetus("UID") + "/KirjattavienKansio" ).toString();
    if( !inbox.isEmpty() && polku.startsWith(inbox))
        inboxista_.append(polku);

    return lisaaHeti( lueTiedosto(polku), QFileInfo(polku).fileName() );
}


bool TositeLiitteet::canDropMimeData(const QMimeData *data, Qt::DropAction /*action*/, int /* row */, int /*column*/, const QModelIndex &/*parent*/) const
{
    return data->hasUrls() || data->formats().contains("image/jpg") || data->formats().contains("image/png");
}

bool TositeLiitteet::dropMimeData(const QMimeData *data, Qt::DropAction action, int /* row */, int /* column */, const QModelIndex & /* parent */)
{
    if( action == Qt::IgnoreAction)
        return true;

    int lisatty = 0;
    // Liitetiedosto pudotettu
    if( data->hasUrls())
    {
        QList<QUrl> urlit = data->urls();
        for(auto url : urlit)
        {
            if( url.isLocalFile())
            {
                QFileInfo info( url.toLocalFile());
                QString polku = info.absoluteFilePath();

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

int TositeLiitteet::tallennettaviaLiitteita() const
{
    int maara = 0;
    for(auto liite : liitteet_)
        if( !liite.getLiiteId() && !liite.getLiitettava())
            maara++;

    return maara;
}

void TositeLiitteet::tallennaLiitteet(int tositeId)
{
    tositeId_ = tositeId;
    tallennuksessa_ = -1;
    tallennaSeuraava();
}

QVariantList TositeLiitteet::liitettavat() const
{
    QVariantList lista;
    for( auto liite: liitteet_)
        if( liite.getLiitettava())
            lista.append(liite.getLiiteId());
    return lista;
}

void TositeLiitteet::naytaLadattuLiite()
{
    naytaLiite_ = true;
}

void TositeLiitteet::nayta(int indeksi)
{
    if( indeksi < 0)
        emit naytaliite( QByteArray());
    else {
        QByteArray sisalto = liitteet_.at(indeksi).getSisalto();
        if(sisalto.isEmpty()) {
            KpKysely* kysely = kpk(QString("/liitteet/%1").arg( liitteet_.at(indeksi).getLiiteId() ));
            connect( kysely, &KpKysely::vastaus, [this, indeksi] (QVariant* data) {this->liitesaapuu(data, indeksi);});
            kysely->kysy();
        } else {
            emit ( naytaliite(sisalto) );
        }
    }
}

void TositeLiitteet::poista(int indeksi)
{
    if( liitteet_.at(indeksi).getLiiteId()) {
        // Liite on tallennettu
        KpKysely* poisto = kpk( QString("/liitteet/%1").arg( liitteet_.at(indeksi).getLiiteId() ), KpKysely::DELETE);
        poisto->kysy();
    }
    inboxista_.removeAll( liitteet_.at(indeksi).getPolku() );   // Ettei poistu inboxista

    beginRemoveRows(QModelIndex(),indeksi, indeksi);
    liitteet_.removeAt(indeksi);    
    endRemoveRows();
}

void TositeLiitteet::tallennaSeuraava()
{
    if( tallennuksessa_ < liitteet_.count() - 1)
    {
        tallennuksessa_++;
        if( !liitteet_.at(tallennuksessa_).getLiiteId() && !liitteet_.at(tallennuksessa_).getLiitettava())
        {
            KpKysely* tallennus = nullptr;
            if( liitteet_.at(tallennuksessa_).getRooli().isEmpty())
                tallennus = kpk( QString("/liitteet/%1").arg(tositeId_), KpKysely::POST );
            else
                tallennus = kpk( QString("/liitteet/%1/%2").arg(tositeId_).arg(liitteet_.at(tallennuksessa_).getRooli()), KpKysely::PUT);

            connect( tallennus, &KpKysely::vastaus, this, &TositeLiitteet::tallennaSeuraava);
            QMap<QString,QString> meta;
            meta.insert("Filename", liitteet_.at(tallennuksessa_).getNimi());
            tallennus->lahetaTiedosto( liitteet_.at(tallennuksessa_).getSisalto(), meta );
            return;
        }
    } else {
        emit liitteetTallennettu();
    }
}

void TositeLiitteet::liitesaapuu(QVariant *data, int indeksi)
{
    liitesaapuuValmiiksi(data, indeksi);
    emit naytaliite( data->toByteArray() );
}

void TositeLiitteet::liitesaapuuValmiiksi(QVariant *data, int indeksi)
{
    // Tässä voisi myös laittaa liitteen muistiin ;)
    if(liitteet_.count() > indeksi)
        liitteet_[indeksi].setSisalto(data->toByteArray());

    emit dataChanged(index(indeksi), index(indeksi), QVector<int>() << Qt::DecorationRole);
}

void TositeLiitteet::liiteLisatty(const QVariant & /*data*/, int liiteId, int liiteIndeksi)
{
    tallennetaan_ = false;
    emit liitettaTallennetaan(false);
    if( liitteet_.size() > liiteIndeksi)
        liitteet_[liiteIndeksi].setLiitettava(liiteId);

}

QByteArray TositeLiitteet::lueTiedosto(const QString &polku)
{
    QByteArray ba;
    QFile tiedosto(polku);
    if( !tiedosto.open(QIODevice::ReadOnly) )
    {
        QMessageBox::critical(nullptr, tr("Tiedostovirhe"),
                              tr("Tiedoston %1 avaaminen epäonnistui \n%2").arg(polku).arg(tiedosto.errorString()));
        return QByteArray();
    }

    ba = tiedosto.readAll();
    tiedosto.close();
    return ba;
}

void TositeLiitteet::liitaFinvoice(const QByteArray &data)
{
    QMap<QString,QString> meta;
    meta.insert("Content-type","application/xml;charset=ISO-8859-15");

    PilviKysely* jsonk = new PilviKysely( kp()->pilvi(), KpKysely::POST,
                                          kp()->pilvi()->finvoiceOsoite() + "/tojson");
    connect( jsonk, &PilviKysely::vastaus, this, &TositeLiitteet::finvoiceJsonSaapuu);
    jsonk->lahetaTiedosto(data, meta);

}

void TositeLiitteet::finvoiceJsonSaapuu(QVariant *data)
{
    emit tuonti(data->toMap());
}

void TositeLiitteet::finvoicePdfSaapuu(QVariant *data)
{
    QByteArray ba = data->toByteArray();
    lisaaHeti(ba,"lasku.pdf");

}


// ************************************ TOSITELIITE **********************************

TositeLiitteet::TositeLiite::TositeLiite(int id, const QString &nimi, const QByteArray &sisalto, const QString &rooli, const QString &polku) :
    liiteId_(id),
    nimi_(nimi),
    rooli_(rooli),
    polku_(polku)
{
    setSisalto(sisalto);
}

int TositeLiitteet::TositeLiite::getLiiteId() const
{
    return liiteId_;
}

void TositeLiitteet::TositeLiite::setLiiteId(int value)
{
    liiteId_ = value;
}

QString TositeLiitteet::TositeLiite::getNimi() const
{
    return nimi_;
}

void TositeLiitteet::TositeLiite::setNimi(const QString &value)
{
    nimi_ = value;
}

QByteArray TositeLiitteet::TositeLiite::getSisalto() const
{
    return sisalto_;
}

QByteArray TositeLiitteet::TositeLiite::getThumb() const
{
    return thumb_;
}

void TositeLiitteet::TositeLiite::setSisalto(const QByteArray &ba)
{
    sisalto_ = ba;

    QString tyyppi = KpKysely::tiedostotyyppi(ba);
    if( tyyppi == "application/pdf") {
        Poppler::Document *pdfDoc = Poppler::Document::loadFromData(ba);
        if( pdfDoc && !pdfDoc->isLocked()) {
            Poppler::Page *sivu = pdfDoc->page(0);
            if( sivu) {
                QImage image = sivu->renderToImage(24,24);
                QPixmap kuva = QPixmap::fromImage(image.scaled(128,128,Qt::KeepAspectRatio));
                QBuffer tallennus(&thumb_);
                tallennus.open(QIODevice::WriteOnly);
                kuva.save(&tallennus, "PNG");
            }
        }
    } else if( tyyppi.startsWith("image/jpeg")) {
        QImage image = QImage::fromData(ba, "JPG");
        QPixmap kuva = QPixmap::fromImage(image.scaled(128,128, Qt::KeepAspectRatio));
        QBuffer tallennus(&thumb_);
        tallennus.open(QIODevice::WriteOnly);
        kuva.save(&tallennus,"PNG");
    }

}

QString TositeLiitteet::TositeLiite::getRooli() const
{
    return rooli_;
}

void TositeLiitteet::TositeLiite::setRooli(const QString &rooli)
{
    rooli_ = rooli;
}

bool TositeLiitteet::TositeLiite::getLiitettava() const
{
    return liitettava_;
}

void TositeLiitteet::TositeLiite::setLiitettava(int id)
{
    liitettava_ = true;
    liiteId_ = id;
}

