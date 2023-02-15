#include "pdftestiapu.h"
#include "../kitsas/tuonti/pdf/pdftiedosto.h"
#include "model/euro.h"

#include <QCoreApplication>
#include <iostream>
#include <QJsonDocument>

TestiApuInfo::TestiApuInfo(const QVariantMap &data)
{
    yTunnus_ = data.value("ytunnus").toString();
    omatNimet_ = data.value("nimi").toStringList();
    omatIbanit_ = data.value("iban").toStringList();
}

Testaaja::Testaaja(const QVariantList &lista, QCoreApplication* app) :
    lista_(lista),
    pdfDoc_(new QPdfDocument(this)),
    app_(app)
{
    connect(pdfDoc_, &QPdfDocument::statusChanged, this, &Testaaja::ladattu);
}

void Testaaja::testaaSeuraava()
{
    indeksi_++;

    if( indeksi_ >= lista_.count()) {
        app_->exit(0);
    } else {
        const QVariantMap& map = lista_.value(indeksi_).toMap();
        const QString tiedosto = map.value("tiedosto").toString();
        std::cout << tiedosto.toStdString() << "\n";

        pdfDoc_->load(tiedosto);
    }
}

void Testaaja::ladattu(QPdfDocument::Status status)
{
    const QVariantMap& map = lista_.value(indeksi_).toMap();

    if( status == QPdfDocument::Status::Ready) {
        qDebug() << map.value("tiedosto").toString();

        TestiApuInfo info(map);

        Tuonti::PdfTiedosto pdfTiedosto(pdfDoc_);

        const QVariantMap& tuotu = pdfTiedosto.tuo(info);


        QVariantMap verrokki = map.value("tulos").toMap();
        QMapIterator<QString,QVariant> iter(verrokki);

        bool ok = true;

        while(iter.hasNext()) {
            iter.next();
            QVariant saatuArvo = tuotu.value(iter.key());
            if( iter.value() != saatuArvo) {
                ok = false;
            }
        }

        if( map.contains("panot") || map.contains("otot")) {
            Euro panot;
            Euro otot;

            for(const auto& item : tuotu.value("tapahtumat").toList()) {
                QVariantMap tmap = item.toMap();
                Euro maara(tmap.value("euro").toString());
                if( maara < Euro::Zero)
                    otot += maara.abs();
                else
                    panot += maara;
            }
            Euro vrtPanot = Euro(map.value("panot").toString());
            Euro vrtOtot = Euro(map.value("otot").toString());

            if( panot != vrtPanot || otot != vrtOtot ) {
                ok = false;
                std::cout << "  PANOT " << panot.display().toStdString() << " / " << vrtPanot.display().toStdString()
                          << "  OTOT  " << otot.display().toStdString() << " / " << vrtOtot.display().toStdString();

            }

        }

        if( !ok ) {
            QJsonDocument doc = QJsonDocument::fromVariant(tuotu);
            std::cout << doc.toJson().toStdString();
        }
        testaaSeuraava();
    } else if( status == QPdfDocument::Status::Error ) {
        qWarning() << " PDF ERROR " << map.value("tiedosto").toString();
        testaaSeuraava();
    }
}
