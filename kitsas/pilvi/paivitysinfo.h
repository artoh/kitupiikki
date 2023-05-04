#ifndef PAIVITYSINFO_H
#define PAIVITYSINFO_H

#include <QObject>
#include "palvelunkertoja.h"
#include "aloitussivu/aloitusinfot.h"
#include <QNetworkReply>

class PaivitysInfo : public QObject, public PalvelunKertoja, public AloitusInfot
{
    Q_OBJECT

protected:

public:
    explicit PaivitysInfo(QObject *parent = nullptr);    
    static QDate buildDate();

signals:
    void infoSaapunut();
    void verkkovirhe(QNetworkReply::NetworkError virhe);

private:
    void pyydaInfo();
    void infoSaapui();

private:
};

#endif // PAIVITYSINFO_H
