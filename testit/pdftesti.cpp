
#include <QCoreApplication>
#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QVariantList>
#include <QCommandLineParser>
#include <iostream>
#include <QTimer>
#include <QFileInfo>
#include <QDir>

#include "pdftestiapu.h"

int main(int argc, char* argv[]) {

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;

    parser.addPositionalArgument("file", "Test json file");
    parser.process(app);

    QString testitiedosto = parser.positionalArguments().value(0);

    std::cerr << "Test file " << testitiedosto.toStdString() << "\n";

    QFile tiedosto(testitiedosto);
    QFileInfo info(tiedosto);
    QDir::setCurrent(info.dir().absolutePath());

    tiedosto.open(QIODevice::ReadOnly);
    QByteArray buff = tiedosto.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(buff);
    QVariantList list = doc.toVariant().toList();

    Testaaja testaaja(list, &app);
    QTimer::singleShot(100, &app, [&testaaja] { testaaja.testaaSeuraava();});

    return app.exec();

}
