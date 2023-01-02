#ifndef VEROVARMENNETILA_H
#define VEROVARMENNETILA_H

#include <QObject>
#include <QDateTime>

class VeroVarmenneTila : public QObject
{
    Q_OBJECT
public:
    explicit VeroVarmenneTila(QObject *parent = nullptr);

    void paivita();
    void tyhjenna();

    void set(const QVariantMap& map);
    QString toString() const;

    QString status() const { return status_;}
    QString errorCode() const { return errorCode_;}
    QDateTime statusTime() const { return statusTime_;}
    bool isValid() const { return status_ == "OK" || status_ == "OF";}

signals:
    void paivitetty();
    void kaytossa();

protected:
    void tilaSaapuu(QVariant* data);

protected:
    QString status_;
    QString errorCode_;
    QDateTime statusTime_;
    QString officeName_;
    QString officeBid_;
};

#endif // VEROVARMENNETILA_H
