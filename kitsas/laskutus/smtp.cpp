/*
Copyright (c) 2013 Raivis Strogonovs
http://morf.lv
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/


#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QApplication>
#include <QSettings>
#include <QDateTime>

#include <QRandomGenerator>
#include <QTimer>

#include "smtp.h"

#include "db/kirjanpito.h"

Smtp::Smtp(const QString &user, const QString &pass, const QString &host, int port, bool encrypted, int timeout )
{
    socket = new QSslSocket(this);

    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(socket, SIGNAL(connected()), this, SLOT(connected() ) );
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this,SLOT(errorReceived(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(disconnected()), this,SLOT(disconnected()));


    this->user = user;
    this->pass = pass;

    this->host = host;
    this->port = port;
    this->timeout = timeout;
    this->encrypted = encrypted;

}



void Smtp::lahetaLiitteella(const QString &from, const QString &to, const QString &subject, const QString &viesti, const QString &liitenimi, const QByteArray &liite)
{
    emit status(Connecting);
    qApp->processEvents();
    emit debug(tr("Yhdistetään palvelimen %1 porttiin %2").arg(host).arg(port));

    message = "To: " + to + "\n";
    message.append("From: " + from + "\n");
    message.append("Subject: =?utf-8?B?" +  subject.toUtf8().toBase64() + "?=\n");
    QString osoite = kp()->asetukset()->asetus("EmailOsoite");
    QString domain = osoite.mid( osoite.indexOf('@') );


    QString mid = "Message-Id: <" +  QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()) + "-" + QString::number(QRandomGenerator::global()->generate64(),16) +
            "-" + kp()->asetukset()->asetus("Ytunnus").left(7) + domain + ">\n";
    message.append( mid );

    message.append("Date: " + QDateTime::currentDateTime().toString(Qt::RFC2822Date) + "\n" );
    message.append("X-Mailer: Kitsas " + qApp->applicationVersion() + "\n");

    //Let's intitiate multipart MIME with cutting boundary "frontier"
    message.append("MIME-Version: 1.0\n");
    message.append("Content-Type: multipart/mixed; boundary=frontier\n\n");

    message.append( "--frontier\n" );
    message.append( "Content-Type: text/plain; charset=\"UTF-8\"\n\n" );  //Uncomment this for HTML formating, coment the line below
    message.append(viesti);

    message.append("\n\n");
    message.append( "--frontier\n" );
    message.append( "Content-Type: application/octet-stream\nContent-Disposition: attachment; filename="+ liitenimi +";\nContent-Transfer-Encoding: base64\n\n" );

    QByteArray liiteBase64 = liite.toBase64();
    for(int i = 0; i < liiteBase64.length(); i += 79) {
        message.append( liiteBase64.mid(i, 79));
        message.append("\n");
    }

    qDebug() << message;

    message.append( "--frontier--\n" );

    message.replace( QString::fromLatin1( "\n" ), QString::fromLatin1( "\r\n" ) );
    message.replace( QString::fromLatin1( "\r\n.\r\n" ),QString::fromLatin1( "\r\n..\r\n" ) );


    this->from = from;
    rcpt = to;
    state = Init;

    // MUOKATTU: Jos portti on 25, toimitaan ilman SSL:n suojaa !
    if( !encrypted )
    {        
        socket->connectToHost(host, port);
        emit debug(tr("Suojaamaton yhteys"));
    }
    else
    {
        socket->connectToHostEncrypted(host, port); //"smtp.gmail.com" and 465 for gmail TLS
        emit debug(tr("Suojattu yhteys"));
    }

    QTimer::singleShot( timeout, this, SLOT(timeOut()) );
}

Smtp::~Smtp()
{
    delete t;
    delete socket;
}
void Smtp::stateChanged(QAbstractSocket::SocketState socketState)
{

    qDebug() <<"stateChanged " << socketState;
    emit debug(tr("(TILA: %1)").arg(socketState));
}

void Smtp::errorReceived(QAbstractSocket::SocketError socketError)
{
    qDebug() << "error " <<socketError;
    // something broke.
    if( state != Close)
    {
        state = Close;
        emit status( Failed);
        emit debug(tr("[Virhe] %1").arg(socket->errorString()));
    }
}

void Smtp::disconnected()
{

    qDebug() <<"disconneted";
    emit debug(tr("Yhteys suljettu"));
}

void Smtp::connected()
{
    qDebug() << "Connected ";
    emit debug(tr("Yhdistetty"));

    t = new QTextStream( socket );
    t->setCodec("UTF-8");
}

void Smtp::readyRead()
{

     qDebug() <<"readyRead";
    // SMTP is line-oriented

    QString responseLine;
    do
    {
        responseLine = socket->readLine();
        response += responseLine;
    }
    while ( socket->canReadLine() && responseLine[3] != ' ' );

    responseLine.truncate( 3 );

    qDebug() << "Server response code:" <<  responseLine;
    qDebug() << "Server response: " << response;
    qDebug() << "State " << state;

    emit debug(tr("[%1] %2").arg(responseLine).arg(response));

    if ( state == Init && responseLine == "220" )
    {
        emit status( Sending );
        if( port == 25)
        {
            // Portti 25 SMTP-protokolla

            *t << "HELO localhost" << "\r\n";
            t->flush();
            state = Mail;
        }
        else
        {
            // banner was okay, let's go on
            *t << "EHLO localhost" <<"\r\n";
            t->flush();

            state = HandShake;
        }
    }
    //No need, because I'm using socket->startClienEncryption() which makes the SSL handshake for you
    /*else if (state == Tls && responseLine == "250")
    {
        // Trying AUTH
        qDebug() << "STarting Tls";
        *t << "STARTTLS" << "\r\n";
        t->flush();
        state = HandShake;
    }*/
    else if (state == HandShake && responseLine == "250")
    {
        socket->startClientEncryption();
        if(!socket->waitForEncrypted(timeout))
        {
            qDebug() << socket->errorString();
            state = Close;
        }


        //Send EHLO once again but now encrypted

        *t << "EHLO localhost" << "\r\n";
        t->flush();
        state = Auth;
    }
    else if (state == Auth && responseLine == "250")
    {
        // Trying AUTH
        qDebug() << "Auth";
        *t << "AUTH LOGIN" << "\r\n";
        t->flush();
        state = User;
    }
    else if (state == User && responseLine == "334")
    {
        //Trying User
        qDebug() << "Username";
        //GMAIL is using XOAUTH2 protocol, which basically means that password and username has to be sent in base64 coding
        //https://developers.google.com/gmail/xoauth2_protocol
        *t << QByteArray().append(user).toBase64()  << "\r\n";
        t->flush();

        state = Pass;
    }
    else if (state == Pass && responseLine == "334")
    {
        //Trying pass
        qDebug() << "Pass";
        *t << QByteArray().append(pass).toBase64() << "\r\n";
        t->flush();

        state = Mail;
    }
    else if ( state == Mail && ( responseLine == "235" || responseLine == "250") )
    {
        // HELO response was okay (well, it has to be)

        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        qDebug() << "MAIL FROM:" << from << "";
        *t << "MAIL FROM: " <<  poimiPelkkaOsoite( from  ) << "\r\n";
        t->flush();
        state = Rcpt;
    }
    else if ( state == Rcpt && responseLine == "250" )
    {
        //Apperantly for Google it is mandatory to have MAIL FROM and RCPT email formated the following way -> <email@gmail.com>
        *t << "RCPT TO: " << poimiPelkkaOsoite( rcpt ) << "\r\n"; //r
        t->flush();
        state = Data;
    }
    else if ( state == Data && responseLine == "250" )
    {

        *t << "DATA\r\n";
        t->flush();
        state = Body;
    }
    else if ( state == Body && responseLine == "354" )
    {
        *t << message << "\r\n.\r\n";
        t->flush();
        state = Quit;
    }
    else if ( state == Quit && responseLine == "250" )
    {

        *t << "QUIT\r\n";
        t->flush();
        // here, we just close.
        state = Close;
        emit status( Send );
        emit debug(tr("QUIT"));
    }
    else if ( state == Close )
    {
        deleteLater();
        return;
    }
    else
    {
        emit status( Failed );
        qDebug() << "BROKEN " << responseLine;
        // something broke.        
        state = Close;
    }
    response = "";
}

void Smtp::timeOut()
{
    if( state == Connecting) {
        state = Close;
        emit status(Failed);
        emit debug(tr("[Virhe] %1").arg(socket->errorString()));
        deleteLater();
    }
}

QString Smtp::poimiPelkkaOsoite(const QString osoite)
{
    QRegularExpression re("<.*@.*>");
    QRegularExpressionMatch mats = re.match(osoite);
    if( mats.hasMatch() )
        return mats.captured(0);
    return QString();
}
