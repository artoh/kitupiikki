#include "emailasetukset.h"
#include "db/kirjanpito.h"
#include "db/asetusmodel.h"
#include "pilvi/pilvimodel.h"

#include "smtpclient/smtpclient.h"

#include <QSettings>
#include <QSslSocket>

EmailAsetukset::EmailAsetukset()
{
    lataa();
}

void EmailAsetukset::setLahettajaOsoite(QString osoite)
{
    osoite.remove(' ');
    lahettajaOsoite_ = osoite;
}

void EmailAsetukset::lataa()
{
    if( !kp()->yhteysModel() ) {
        lataaTietokoneAsetukset();
    } else if ( kp()->asetukset()->onko("KitsasEmail") ) {
        lataaKitsasAsetukset();
    } else if( !kp()->asetukset()->asetus(AsetusModel::SmtpServer).isEmpty()) {
        lataaKirjanpitoAsetukset();
    } else {
        lataaTietokoneAsetukset();
    }
}

void EmailAsetukset::tallenna()
{
    if( asetustapa_ == ASETUKSET_TIETOKONE ) {
        tallennaPaikallinen(); }
    else {
        tallennaKirjanpito();
    }
    kp()->asetukset()->aseta("KitsasEmail",  kitsasPalvelin() );
}

void EmailAsetukset::lataaTietokoneAsetukset()
{
    asetustapa_ = ASETUKSET_TIETOKONE;
    palvelin_ = kp()->settings()->value("SmtpServer").toString();
    kayttaja_ = kp()->settings()->value("SmtpUser").toString();
    salasana_ = kp()->settings()->value("SmtpPassword").toString();
    portti_ = kp()->settings()->value("SmtpPort", QSslSocket::supportsSsl() ? 465 : 25).toInt();
    suojaus_ = sslIndeksi( kp()->settings()->value("EmailSSL").toString() );

    if( !lataaOmaEmail()) {
        lahettajaNimi_ = kp()->settings()->value("EmailNimi").toString();
        lahettajaOsoite_ = kp()->settings()->value("EmailOsoite").toString();
    }

    kopioOsoite_ = kp()->settings()->value("EmailKopio").toString();
}

void EmailAsetukset::lataaKirjanpitoAsetukset()
{
    asetustapa_ = ASETUKSET_KIRJANPITO;
    palvelin_ =  kp()->asetukset()->asetus(AsetusModel::SmtpServer);
    kayttaja_ = kp()->asetukset()->asetus(AsetusModel::SmtpUser);
    salasana_ = kp()->asetukset()->asetus(AsetusModel::SmtpPassword);
    portti_ = kp()->asetukset()->luku(AsetusModel::SmtpPort, QSslSocket::supportsSsl() ? 465 : 25);
    suojaus_ = sslIndeksi( kp()->asetukset()->asetus(AsetusModel::EmailSSL) );

    if(!lataaOmaEmail()) {
        lahettajaOsoite_ = kp()->asetukset()->asetus(AsetusModel::EmailOsoite);
        lahettajaNimi_ = kp()->asetukset()->asetus(AsetusModel::EmailNimi);
    }
    kopioOsoite_ = kp()->asetukset()->asetus(AsetusModel::EmailKopio);
}

void EmailAsetukset::lataaKitsasAsetukset()
{
    lataaKirjanpitoAsetukset();
    asetustapa_ = KITSAS_PALVELIN;
}


bool EmailAsetukset::lataaOmaEmail()
{
    const int kayttajaId = kp()->pilvi()->kayttajaPilvessa();
    if( kayttajaId ) {
        const QString omaEmail = kp()->asetukset()->asetus( QString("OmaEmail/%1").arg(kp()->pilvi()->kayttajaPilvessa()) );
        if( !omaEmail.isEmpty() ) {
            kayttajakohtainen_ = true;
            const int vali = omaEmail.indexOf(' ');
            lahettajaOsoite_ =  omaEmail.left(vali);
            lahettajaNimi_ = omaEmail.mid(vali + 1) ;
            return true;
        }
    }
    kayttajakohtainen_ = false;
    return false;
}

void EmailAsetukset::tallennaPaikallinen()
{
    kp()->settings()->setValue("SmtpServer",  palvelin() );
    kp()->settings()->setValue("SmtpPort", portti() );
    kp()->settings()->setValue("SmtpUser", kayttaja() );
    kp()->settings()->setValue("SmtpPassword", salasana() );
    kp()->settings()->setValue("EmailSSL", sslAsetus( suojaus() ));
    if( !tallennaOmaEmail()) {
        kp()->settings()->setValue("EmailNimi", lahettajaNimi());
        kp()->settings()->setValue("EmailOsoite", lahettajaOsoite());
    }
    kp()->settings()->setValue("EmailKopio", kopioOsoite() );

    if( kp()->yhteysModel() ) {
        kp()->asetukset()->poista("SmtpServer");
    }
}

void EmailAsetukset::tallennaKirjanpito()
{
    kp()->asetukset()->aseta(AsetusModel::SmtpServer, palvelin() );
    kp()->asetukset()->aseta(AsetusModel::SmtpPort, portti() );
    kp()->asetukset()->aseta(AsetusModel::SmtpUser, kayttaja() );
    kp()->asetukset()->aseta(AsetusModel::SmtpPassword,  salasana() );
    kp()->asetukset()->aseta(AsetusModel::EmailSSL, sslAsetus( suojaus() ));

    if(!tallennaOmaEmail()) {
        kp()->asetukset()->aseta(AsetusModel::EmailOsoite,  lahettajaOsoite() );
        kp()->asetukset()->aseta(AsetusModel::EmailNimi, lahettajaNimi() );
    }

    kp()->asetukset()->aseta(AsetusModel::EmailKopio, kopioOsoite() );

}

bool EmailAsetukset::tallennaOmaEmail()
{
    if( kayttajakohtainen() ) {
        kp()->asetukset()->aseta(QString("OmaEmail/%1").arg(kp()->pilvi()->kayttajaPilvessa()),
                                 lahettajaOsoite() + " " + lahettajaNimi());
        return true;
    } else {
        if( kp()->yhteysModel()) {
            kp()->asetukset()->poista(QString("OmaEmail/%1").arg(kp()->pilvi()->kayttajaPilvessa()));
        }
        return false;
    }
}

int EmailAsetukset::sslIndeksi(const QString &asetus)
{
    if( asetus == "STARTTLS")
        return SmtpClient::TlsConnection;
    else if(asetus == "ON")
        return SmtpClient::SslConnection;
    else
        return SmtpClient::TcpConnection;
}

QString EmailAsetukset::sslAsetus(int indeksi)
{
    switch (indeksi) {
        case SmtpClient::TcpConnection: return "EI";
        case SmtpClient::SslConnection: return "ON";
        case SmtpClient::TlsConnection: return "STARTTLS";
    }
    return QString();
}
