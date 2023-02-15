#ifndef TUONTI_TUONTIAPUINFO_H
#define TUONTI_TUONTIAPUINFO_H

#include <QStringList>


namespace Tuonti {

class TuontiApuInfo
{
public:
    TuontiApuInfo();

    bool omaNimi(const QString& nimi) const;
    bool omaYtunnus(const QString& ytunnus) const;
    bool omaIban(const QString& iban) const;

protected:
    QStringList omatNimet_;
    QString yTunnus_;
    QStringList omatIbanit_;
};

} // namespace Tuonti

#endif // TUONTI_TUONTIAPUINFO_H
