#include "paivitysinfo.h"

#include "db/kirjanpito.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QSettings>
#include <QApplication>
#include "kieli/kielet.h"
#include "pilvi/pilvimodel.h"
#include "versio.h"
#include "aloitussivu/loginservice.h"

#include <QTimer>

PaivitysInfo::PaivitysInfo(QObject *parent)
    : QObject{parent}
{
    QTimer::singleShot(100, this, &PaivitysInfo::pyydaInfo);
}

QDate PaivitysInfo::buildDate()
{
    QString koostepaiva(__DATE__);      // Tämä päivittyy aina versio.h:ta muutettaessa
    return QDate::fromString( koostepaiva.mid(4,3) + koostepaiva.left(3) + koostepaiva.mid(6), Qt::RFC2822Date);

}

void PaivitysInfo::pyydaInfo()
{
    QVariantMap tilasto;
    if( kp()->settings()->contains("TilastoPaivitetty")) {
        tilasto.insert("lastasked", kp()->settings()->value("TilastoPaivitetty").toDate());
    }
    tilasto.insert("application", qApp->applicationName());
    tilasto.insert("version", qApp->applicationVersion());
    tilasto.insert("build", KITSAS_BUILD);
    tilasto.insert("os", QSysInfo::prettyProductName());
    tilasto.insert("language", Kielet::instanssi()->uiKieli());
    tilasto.insert("builddate", buildDate().toString(Qt::ISODate));

    QByteArray ba = QJsonDocument::fromVariant(tilasto).toJson();
    QString osoite = kp()->pilvi()->pilviLoginOsoite() + "/updateinfo";

    QNetworkRequest pyynto = QNetworkRequest( QUrl(osoite));
    pyynto.setSslConfiguration(QSslConfiguration::defaultConfiguration());
    pyynto.setRawHeader("Content-type","application/json");
    pyynto.setRawHeader("User-Agent", QString("%1 %2 %3").arg(qApp->applicationName(),qApp->applicationVersion(), QSysInfo::prettyProductName()).toUtf8()  );
    QNetworkReply *reply = kp()->networkManager()->post(pyynto, ba);
    connect( reply, &QNetworkReply::finished, this, &PaivitysInfo::infoSaapui);

}

void PaivitysInfo::infoSaapui()
{
   QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
   if( !reply->error()) {

        QVariantMap map = QJsonDocument::fromJson( reply->readAll() ).toVariant().toMap();

        Kirjanpito::asetaOhjeOsoite( map.value("docs").toString() );
        setService("feedback", map.value("feedback").toString());
        kp()->settings()->setValue("TilastoPaivitetty", QDate::currentDate());

        asetaInfot( map.value("info").toList());
        emit infoSaapunut();
   } else {
        QNetworkReply::NetworkError error = reply->error();
        info( "varoitus", tr("Palvelimeen ei saada yhteyttä"), LoginService::verkkovirheteksti(error, reply->errorString()),
             QString(), "verkkovirhe.png");
        emit infoSaapunut();
        emit verkkovirhe(error);
   }
    reply->deleteLater();
}
