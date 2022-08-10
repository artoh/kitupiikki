#ifndef TILITIETO_PANKKI_H
#define TILITIETO_PANKKI_H

#include <QVariantMap>
#include <QObject>
#include <QImage>

namespace Tilitieto {

class Pankki : public QObject {
    Q_OBJECT
public:

    Pankki(QObject *parent = nullptr);
    Pankki(QVariantMap map, QObject *parent);

    int id() const { return id_;}
    QString nimi() const { return nimi_; }
    QString bic() const { return bic_;}
    QImage logo() const { return logo_;}
    QIcon icon() const;

private:
    void logoSaapuu();

    int id_;
    QString nimi_;
    QString bic_;
    QImage logo_;
};

} // namespace Tilitieto

#endif // TILITIETO_PANKKI_H
