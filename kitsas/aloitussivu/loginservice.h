#ifndef LOGINSERVICE_H
#define LOGINSERVICE_H

#include <QObject>
#include <QRegularExpression>
#include <QNetworkReply>
#include "pilvi/pilvimodel.h"
#include "pilvi/pilvikayttaja.h"

class LoginService : public QObject
{
    Q_OBJECT
public:
    explicit LoginService(QWidget *parent = nullptr);

    void setEmail(const QString& email);
    void setPassword(const QString& password);
    void login(bool remember = false);
    void forgetPassword();
    void clear();

signals:
    void passwdAllowed(bool allow);
    void loginAllowed(bool allow);
    void incorrectPassword();
    void networkError(const QString& message);
    void logged(PilviKayttaja kayttaja);
    void changeMailed(int error);

private:
    void tarkastaEmail();
    void emailTarkastettu();
    void verkkovirhe(QNetworkReply::NetworkError virhe);
    void ilmoitaTilat();
    void vaihtoLahti();

private:
    QString email_;
    QString password_;
    bool emailOk_ = false;


public:
    static QRegularExpression emailRE;

};

#endif // LOGINSERVICE_H
