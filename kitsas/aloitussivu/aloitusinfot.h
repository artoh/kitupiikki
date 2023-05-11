#ifndef ALOITUSINFOT_H
#define ALOITUSINFOT_H

#include "aloitusinfo.h"
#include <QList>

class AloitusInfot
{
public:
    AloitusInfot();

    QString toHtml() const;
    void asetaInfot(const QVariantList& data);

    void info(const QString& luokka, const QString& otsikko, const QString& teksti,
              const QString& linkki = QString(), const QString kuva = QString(), const QString ohjelinkki = QString(), int id=0);

    void clear();

    void poistaNotify(int notifyId);

private:
    QList<AloitusInfo> infot_;
};

#endif // ALOITUSINFOT_H
