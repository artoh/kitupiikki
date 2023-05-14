#ifndef LOGINSERVICE_H
#define LOGINSERVICE_H

#include <QObject>
#include <QRegularExpression>
#include <QNetworkReply>
#include "pilvi/pilvikayttaja.h"

class QLineEdit;
class QPushButton;
class QCheckBox;
class QLabel;

class LoginService : public QObject
{
    Q_OBJECT
public:
    explicit LoginService(QWidget *parent = nullptr);

    void registerWidgets( QLineEdit* emailEdit, QLineEdit* passwordEdit,
                          QLabel* messageLabel, QCheckBox* rememberBox,
                          QPushButton* loginButon, QPushButton* forgotButton);

    void keyLogin();

    static QString verkkovirheteksti(QNetworkReply::NetworkError virhe, const QString &virheTeksti);

signals:
    void logged(PilviKayttaja kayttaja);    


private:
    void emailMuokattu();
    void tarkastaEmail();
    void emailTarkastettu();
    void paivitaTilat();

    void login();
    void loginVastaus();

    void request2fa(const QVariantMap& map);

    void forgetPassword();
    void verkkovirhe(QNetworkReply::NetworkError virhe);
    void vaihtoLahti();

    void kirjauduttu(PilviKayttaja kayttaja);

    void auth(QVariantMap map);


protected:
    QLineEdit* emailEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLabel* messageLabel_ = nullptr;
    QCheckBox* rememberBox_ = nullptr;
    QPushButton* loginButton_ = nullptr;
    QPushButton* forgetButton_ = nullptr;

private:
    bool emailOk_ = false;


public:
    static QRegularExpression emailRE;

};

#endif // LOGINSERVICE_H
