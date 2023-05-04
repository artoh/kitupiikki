#include "loginservice.h"
#include "db/kirjanpito.h"
#include "pilvi/pilvimodel.h"


#include <QApplication>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QWidget>
#include <QMessageBox>

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QSettings>

#include <QRegularExpressionValidator>

#include "versio.h"
#include "kaksivaihedialog.h"
#include "pilvi/paivitysinfo.h"

LoginService::LoginService(QWidget *parent)
    : QObject{parent}
{
    connect( kp()->pilvi(), &PilviModel::kirjauduttu, this, &LoginService::kirjauduttu);
    connect( kp()->pilvi()->paivitysInfo(), &PaivitysInfo::verkkovirhe, this, &LoginService::verkkovirhe);
}

void LoginService::registerWidgets(QLineEdit *emailEdit, QLineEdit *passwordEdit, QLabel *messageLabel, QCheckBox *rememberBox, QPushButton *loginButon, QPushButton *forgotButton)
{
    emailEdit_ = emailEdit;
    passwordEdit_ = passwordEdit;
    messageLabel_ = messageLabel;
    rememberBox_ = rememberBox;
    loginButton_ = loginButon;
    forgetButton_ = forgotButton;

    emailEdit_->setValidator(new QRegularExpressionValidator(emailRE, this));

    messageLabel_->hide();
    loginButton_->setEnabled(false);
    forgetButton_->setEnabled(false);
    passwordEdit_->setEnabled(false);

    connect( emailEdit_, &QLineEdit::textEdited, this, &LoginService::emailMuokattu);
    connect( passwordEdit_, &QLineEdit::textEdited, this, &LoginService::paivitaTilat);
    connect( loginButton_, &QPushButton::clicked, this, &LoginService::login);
    connect( forgetButton_, &QPushButton::clicked, this, &LoginService::forgetPassword);
}

void LoginService::emailMuokattu()
{
    emailOk_ = false;
    passwordEdit_->setPlaceholderText(QString());
    paivitaTilat();
    if( emailRE.match( emailEdit_->text() ).hasMatch() && !emailEdit_->text().endsWith("gmail.co")) {
        tarkastaEmail();
    } else {
        messageLabel_->hide();
    }
}


void LoginService::forgetPassword()
{
    QVariantMap map;

    map.insert("email", emailEdit_->text());
    QNetworkAccessManager *mng = kp()->networkManager();

    QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users") );

    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());

    QNetworkReply *reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::finished, this, &LoginService::vaihtoLahti);
}


void LoginService::tarkastaEmail()
{    
    QNetworkRequest request( QUrl( kp()->pilvi()->pilviLoginOsoite() + "/users/" + emailEdit_->text() ));
    request.setRawHeader("User-Agent", QString(qApp->applicationName() + " " + qApp->applicationVersion()).toUtf8());
    QNetworkReply *reply =  kp()->networkManager()->get(request);
    connect( reply, &QNetworkReply::finished, this, &LoginService::emailTarkastettu);
    connect( reply, &QNetworkReply::errorOccurred, this, &LoginService::verkkovirhe);
}

void LoginService::emailTarkastettu()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    emailOk_ = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
    if( emailOk_ ) {
        passwordEdit_->setPlaceholderText("Kirjoita nyt salasana");
        messageLabel_->hide();
    } else if( reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404 ) {
        messageLabel_->setText("Käyttäjätunnusta ei löydy");
        messageLabel_->setStyleSheet("color: orangered");
        messageLabel_->show();
    }
    paivitaTilat();
}

void LoginService::paivitaTilat()
{
    passwordEdit_->setEnabled( emailOk_ );
    forgetButton_->setEnabled( emailOk_);

    bool passOk = passwordEdit_->text().length() >= 10;
    loginButton_->setEnabled( emailOk_ && passOk );
    rememberBox_->setEnabled( emailOk_ && passOk );

}

void LoginService::login()
{
    QVariantMap map;
    map.insert("email", emailEdit_->text());
    map.insert("password", passwordEdit_->text());
    map.insert("requestKey", rememberBox_->isChecked());

    messageLabel_->hide();

    auth(map);
}

void LoginService::keyLogin()
{
    if( kp()->settings()->contains("AuthKey") && !kp()->pilvi()->kayttaja() ) {
        QVariantMap map;

        QVariantMap keyMap;
        QStringList keyData = kp()->settings()->value("AuthKey").toString().split(",");
        keyMap.insert("id",keyData.value(0));
        keyMap.insert("secret", keyData.value(1));
        map.insert("key", keyMap);
        map.insert("requestKey",true);
        kp()->settings()->remove("AuthKey");

        auth(map);
    }
}

QString LoginService::verkkovirheteksti(QNetworkReply::NetworkError virhe)
{
    if( virhe == QNetworkReply::ConnectionRefusedError)
        return tr("Palvelin ei juuri nyt ole käytettävissä. Yritä myöhemmin uudelleen.");
    else if( virhe == QNetworkReply::TemporaryNetworkFailureError || virhe == QNetworkReply::NetworkSessionFailedError)
        return("Verkkoon ei saada yhteyttä");
    else if( virhe == QNetworkReply::HostNotFoundError)
        return("Palvelinta ei löydetä. Tarkasta verkkoyhteys.");
    else if( virhe == QNetworkReply::UnknownServerError)
        return tr("Palvelu on tilapäisesti poissa käytöstä.");
    else
        return tr("Palvelinyhteydessä on virhe (%1)").arg(virhe);
}

void LoginService::loginVastaus()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QWidget* myParent = qobject_cast<QWidget*>(parent());
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if( status == 401) {
        messageLabel_->setText(tr("Virheellinen salasana"));
        messageLabel_->setStyleSheet("color: red;");
        messageLabel_->show();
        passwordEdit_->clear();
        return;
    }

    if( status != 200) {
        QMessageBox::critical(myParent, tr("Virhe kirjautumisessa"),
                              tr("Palvelin ilmoitti virheen %1").arg(status));

        return;
    }

    QByteArray vastaus = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( vastaus );
    QVariantMap map = doc.toVariant().toMap();

    if( map.contains("request2fa")) {
        request2fa(map);
    } else {
        int viimeisin = kp()->settings()->value("Viimeisin").toInt();

        // Kirjaudutaan ja avataan viimeisin kirjanpito
        kp()->pilvi()->kirjautuminen( map, viimeisin && !kp()->yhteysModel() ? viimeisin : 0);

        emailEdit_->clear();
        passwordEdit_->clear();

    }
}

void LoginService::request2fa(const QVariantMap &map)
{
    QWidget* myParent = qobject_cast<QWidget*>(parent());
    KaksivaiheDialog dlg(myParent);
    const QString code = dlg.askCode( map.value("name").toString() );
    if( !code.isEmpty()) {
        QVariantMap tmap;
        tmap.insert("code", code);
        tmap.insert("key", map.value("request2fa"));
        tmap.insert("requestKey", map.value("requestKey").toBool());

        auth(tmap);
    }

}

void LoginService::verkkovirhe(QNetworkReply::NetworkError virhe)
{    
    if( virhe == QNetworkReply::ContentNotFoundError && !emailOk_)
        return; // Ei ole virhe jos ei löydä sähköpostia

    const QString txt = verkkovirheteksti(virhe);

    if( virhe < QNetworkReply::ContentAccessDenied || virhe == QNetworkReply::UnknownServerError) {
        QTimer::singleShot(30000, this, &LoginService::tarkastaEmail);
    }

    qWarning() << "LoginService verkkovirhe " << virhe << " " << txt;

    messageLabel_->setText(QString("<b>%1</b>").arg(txt));
    messageLabel_->setStyleSheet("background-color: yellow;");
    messageLabel_->show();
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

void LoginService::kirjauduttu(PilviKayttaja kayttaja)
{
    if( kayttaja ) {
        emit logged(kayttaja);
    }
}

void LoginService::auth(QVariantMap map)
{
    map.insert("application", qApp->applicationName());
    map.insert("version", qApp->applicationVersion());
    map.insert("build", KITSAS_BUILD);
    map.insert("os", QSysInfo::prettyProductName());

    QNetworkAccessManager *mng = kp()->networkManager();

    QNetworkRequest request(QUrl( kp()->pilvi()->pilviLoginOsoite() + "/auth") );
    request.setRawHeader("Content-Type","application/json");
    request.setRawHeader("User-Agent",QString(qApp->applicationName() + " " + qApp->applicationVersion() ).toLatin1());

    QNetworkReply* reply = mng->post( request, QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact) );
    connect( reply, &QNetworkReply::errorOccurred, this, &LoginService::verkkovirhe);
    connect( reply, &QNetworkReply::finished, this, &LoginService::loginVastaus);
}




QRegularExpression LoginService::emailRE{QRegularExpression(R"(^[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+[.][A-Za-z]{2,64}$)", QRegularExpression::UseUnicodePropertiesOption)};

