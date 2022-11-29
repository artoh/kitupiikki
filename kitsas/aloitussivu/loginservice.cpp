#include "loginservice.h"
#include "db/kirjanpito.h"


#include <QApplication>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QWidget>
#include <QMessageBox>


LoginService::LoginService(QWidget *parent)
    : QObject{parent}
{
    connect( kp()->pilvi(), &PilviModel::loginvirhe, this, &LoginService::incorrectPassword);
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &LoginService::logged);
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &LoginService::clear);
}

void LoginService::setEmail(const QString &email)
{
    email_ = email;
    emailOk_ = false;
    ilmoitaTilat();
    if( emailRE.match( email_ ).hasMatch()) {
        tarkastaEmail();
    }
}

void LoginService::setPassword(const QString &password)
{
    password_ = password;
    ilmoitaTilat();
}

void LoginService::login(bool remember)
{
    kp()->pilvi()->kirjaudu(email_, password_, remember);
}

void LoginService::forgetPassword()
{
    QVariantMap map;

    map.insert("email", email_);
    QNetworkAccessManager *mng = kp()->networkManager();

    QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users") );

    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &LoginService::vaihtoLahti);
}

void LoginService::clear()
{
    email_.clear();
    password_.clear();

    ilmoitaTilat();
    emit networkError(QString());
}

void LoginService::tarkastaEmail()
{
    QNetworkRequest request( QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users/" + email_ ));
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
    QNetworkReply *reply =  kp()->networkManager()->get(request);
    connect( reply, &QNetworkReply::finished, this, &LoginService::emailTarkastettu);
    connect( reply, &QNetworkReply::errorOccurred, this, &LoginService::verkkovirhe);
}

void LoginService::emailTarkastettu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    emailOk_ = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
    emit networkError(QString());
    ilmoitaTilat();
}

void LoginService::verkkovirhe(QNetworkReply::NetworkError virhe)
{
    QString txt;
    if( virhe == QNetworkReply::ConnectionRefusedError)
        txt = tr("Palvelin ei juuri nyt ole käytettävissä. Yritä myöhemmin uudelleen.");
    else if( virhe == QNetworkReply::TemporaryNetworkFailureError || virhe == QNetworkReply::NetworkSessionFailedError)
        txt = tr("Verkkoon ei saada yhteyttä");
    else if(virhe < QNetworkReply::ContentAccessDenied )
        txt = tr("Palvelinyhteydessä on virhe (%1)").arg(virhe);
    else if( virhe == QNetworkReply::UnknownServerError)
        txt = tr("<p><b>Palvelu on tilapäisesti poissa käytöstä.</b>");

    if( virhe < QNetworkReply::ContentAccessDenied || virhe == QNetworkReply::UnknownServerError) {
        QTimer::singleShot(30000, this, &LoginService::tarkastaEmail);
    }

    emit networkError(txt);
}

void LoginService::vaihtoLahti()
{
    QWidget* myParent = qobject_cast<QWidget*>(parent());
    QNetworkReply *reply = qobject_cast<QNetworkReply*>( sender());
    if( reply->error()) {
        QMessageBox::critical(myParent, tr("Salasanan vaihtaminen epäonnistui"),
            tr("Salasanan vaihtopyynnön lähettäminen palvelimelle epäonnistui "
               "tietoliikennevirheen %1 takia.\n\n"
               "Yritä myöhemmin uudelleen").arg( reply->error() ));
        return;
    }
    QMessageBox::information(myParent, tr("Salasanan palauttaminen"),
                 tr("Sähköpostiisi on lähetetty linkki, jonka avulla "
                    "voit vaihtaa salasanan."));

}

void LoginService::ilmoitaTilat()
{
    emit passwdAllowed( emailOk_ );
    emit loginAllowed( emailOk_ && password_.length() >= 10 );
}


QRegularExpression LoginService::emailRE{QRegularExpression(R"(^[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+[.][A-Za-z]{2,64}$)")};

