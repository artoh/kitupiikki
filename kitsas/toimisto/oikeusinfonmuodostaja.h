#ifndef OIKEUSINFONMUODOSTAJA_H
#define OIKEUSINFONMUODOSTAJA_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QMap>

class OikeusInfonMuodostaja : public QObject
{
    Q_OBJECT

protected:
    class OikeusOtsikko {
    public:
        OikeusOtsikko(const QString& nimi);
        void lisaa(const QString& lyhenne, const QString& selite);
        QString muodosta(const QStringList& oikeudet) const;

    private:
        QString nimi_;
        QList<QPair<QString,QString>> lista_;
    };


public:
    static QString oikeusinfo(const QStringList& oikeudet);

private:
    OikeusInfonMuodostaja();
    QString muodosta(const QStringList& oikeudet);

    QList<OikeusOtsikko> otsikot_;
    static OikeusInfonMuodostaja* instanssi__;
};

#endif // OIKEUSINFONMUODOSTAJA_H
