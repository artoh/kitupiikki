#ifndef TILITIETO_PANKKI_H
#define TILITIETO_PANKKI_H

#include <QVariantMap>
#include <QImage>

namespace Tilitieto {

class Pankki {

public:         
    Pankki();
    Pankki(const QVariantMap &map);

    int id() const { return id_;}
    QString nimi() const { return nimi_; }
    QString bic() const { return bic_;}
    QImage logo() const { return logo_;}
    QIcon icon() const;

    QString logoUrl() const { return logoUrl_; }
    void setLogo(const QImage& logo);


private:
    int id_ = 0;
    QString nimi_;
    QString bic_;
    QString logoUrl_;
    QImage logo_;
};

} // namespace Tilitieto

#endif // TILITIETO_PANKKI_H
