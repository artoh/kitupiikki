#ifndef TESTIAPUINFO_H
#define TESTIAPUINFO_H


#include "../kitsas/tuonti/pdf/tuontiapuinfo.h"

#include <QVariantMap>
#include <QPdfDocument>
#include <QCoreApplication>

class TestiApuInfo : public Tuonti::TuontiApuInfo {

public:
    TestiApuInfo(const QVariantMap& data);

};


class Testaaja : public QObject {
    Q_OBJECT

public:
    Testaaja(const QVariantList& lista, QCoreApplication* app);

    void testaaSeuraava();
    void ladattu(QPdfDocument::Status status);

protected:
    QVariantList lista_;
    QPdfDocument* pdfDoc_;
    QCoreApplication* app_;

    int indeksi_ = -1;

};


#endif
