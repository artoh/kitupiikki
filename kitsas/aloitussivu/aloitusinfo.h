#ifndef ALOITUSINFO_H
#define ALOITUSINFO_H

#include <QString>
#include <QVariantMap>

class AloitusInfo
{
public:
    AloitusInfo();
    AloitusInfo(const QVariantMap& map);
    AloitusInfo(const QString& luokka, const QString& otsikko, const QString& teksti,
                const QString& linkki = QString(), const QString kuva = QString(), const QString ohjelinkki = QString(),
                const int id = 0);
    QString toHtml() const;

    int notifyId() const { return notifyId_;}

protected:
    QString luokka_;
    QString otsikko_;
    QString teksti_;
    QString linkki_;
    QString kuva_;
    QString ohjelinkki_;
    int notifyId_ = 0;

};

#endif // ALOITUSINFO_H
