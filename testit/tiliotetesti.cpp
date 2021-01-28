#include "../kitsas/tuonti/pdftuonti.h"
#include <iostream>
#include <QFile>

#include <QtCore>
#include <QByteArray>
#include <QTextStream>

#include <QJsonDocument>
#include <QDebug>

using namespace std;

int main(int /* argc */, char * /* argv */[]) {

    int oikein = 0;
    int vaarin = 0;

    const QString POLKU = "/home/arto/tiliotetesti/";

    QFile tiedosto(POLKU + "otteet.txt");
    tiedosto.open(QIODevice::ReadOnly);
    QTextStream stream(&tiedosto);
    QString kaikki = stream.readAll();
    QStringList rivit = kaikki.split("\n");
    for(QString &rivi : rivit) {
        QStringList osat = rivi.split(' ');
        QString tiedosto = osat.value(0);

        QFile tote(POLKU + tiedosto);
        if(tote.open(QIODevice::ReadOnly)) {
            QByteArray ba = tote.readAll();
            QVariantMap map = Tuonti::PdfTuonti::tuo(ba);

            qlonglong panot = 0;
            qlonglong otot = 0;

            QVariantList tapahtumat = map.value("tapahtumat").toList();
            for(auto& item : tapahtumat) {
                QVariantMap tmap = item.toMap();
                qlonglong eurot = qRound64( tmap.value("euro").toDouble() * 100.0 );
                if( eurot > 0)
                    panot += eurot;
                else
                    otot -= eurot;
            }

            qlonglong ppanot = qRound64( osat.value(1).toDouble() * 100.0 );
            qlonglong potot = qRound64( osat.value(2).toDouble() * 100.0);

            if( panot == ppanot && otot == potot) {
                oikein++;
            } else {
                vaarin++;
                cout << "\n ************************* " << tiedosto.toStdString() << " ***************************\n";
                cout << QString(" PANOT %L1 / %L2  (%L5) OTOT %L3 / %L4 (%L6) \n\n")
                        .arg(panot / 100.0, 0, 'f',2)
                        .arg(ppanot / 100.0,0, 'f',2)
                        .arg(otot / 100.0,0, 'f',2)
                        .arg(potot / 100.0,0, 'f',2)
                        .arg((panot - ppanot) / 100.0,0, 'f',2)
                        .arg((otot - potot) / 100.0,0, 'f',2)
                        .toStdString();

                for(auto& item : tapahtumat) {
                    QVariantMap tmap = item.toMap();
                    cout << QString("%1 %L2  %3 %4 %5 \n")
                            .arg(tmap.value("pvm").toDate().toString("dd.MM."))
                            .arg(tmap.value("euro").toDouble(), 12, 'f', 2)
                            .arg(tmap.value("saajamaksaja").toString(),-40)
                            .arg(tmap.value("viite").toString(), -10)
                            .arg(tmap.value("selite").toString())
                            .toStdString();
                }

                // cout << QJsonDocument::fromVariant(map).toJson().toStdString();
            }
        }
    }
    cout << "\n\n Onnistui " << oikein << " / " << oikein + vaarin << "\n";

}
