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
              const QString& linkki = QString(), const QString kuva = QString(), const QString ohjelinkki = QString(), const QString &id=QString());

    void clear();

    void poistaNotify(const QString &notifyId);

private:
    QList<AloitusInfo> infot_;
};

#endif // ALOITUSINFOT_H
