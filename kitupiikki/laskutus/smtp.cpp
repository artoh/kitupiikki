/*
Copyright (c) 2013 Raivis Strogonovs
http://morf.lv
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/


#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QApplication>

#include "smtp.h"

Smtp::Smtp( const QString &user, const QString &pass, const QString &host, int port, int timeout )
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


}



void Smtp::lahetaLiitteella(const QString &from, const QString &to, const QString &subject, const QString &viesti, const QString &liitenimi, const QByteArray &liite)
{
    emit status(tr("Yhdistetään sähköpostipalvelimeen..."));
    qApp->processEvents();

    otsikko = "To: " + to + "\n";
    otsikko.append("From: " + from + "\n");
    otsikko.append("Subject: " + subject + "\n");

    //Let's intitiate multipart MIME with cutting boundary "frontier"
    otsikko.append("MIME-Version: 1.0\n");
    otsikko.append("Content-Type: multipart/mixed; boundary=frontier\n\n");

    otsikko.append( "--frontier\n" );
    otsikko.append( "Content-Type: text/html; charset=\"UTF-8\"\n\n" );  //Uncomment this for HTML formating, coment the line below
    // message.append( "Content-Type: text/plain\n\n" );
    message = viesti;

    message.append("\n\n");
    message.append( "--frontier\n" );
    message.append( "Content-Type: application/octet-stream\nContent-Disposition: attachment; filename="+ liitenimi +";\nContent-Transfer-Encoding: base64\n\n" );
    message.append(liite.toBase64());
    message.append("\n");

    message.append( "--frontier--\n" );

    message.replace( QString::fromLatin1( "\n" ), QString::fromLatin1( "\r\n" ) );
    message.replace( QString::fromLatin1( "\r\n.\r\n" ),QString::fromLatin1( "\r\n..\r\n" ) );


    this->from = from;
    rcpt = to;
    state = Init;

    // MUOKATTU: Jos portti on 25, toimitaan ilman SSL:n suojaa !
    if( port == 25)
    {
        socket->connectToHost(host, port);
    }
    else
    {
        socket->connectToHostEncrypted(host, port); //"smtp.gmail.com" and 465 for gmail TLS
    }


    if (!socket->waitForConnected(timeout)) {

        if( state != Close)
        {
            QMessageBox::warning( nullptr, tr( "Virhe sähköpostin lähetyksessä" ), tr( "Sähköpostipalvelin ilmoitti virheen: %1" ).arg( socket->errorString())  );
            state = Close;
            emit status(tr("Sähköpostin lähetys epäonnistui"));
        }
        qDebug() << socket->errorString();
     }


    t = new QTextStream( socket );
    t->setCodec("ISO 8859-1");

}

Smtp::~Smtp()
{
    delete t;
    delete socket;
}
void Smtp::stateChanged(QAbstractSocket::SocketState socketState)
{

    qDebug() <<"stateChanged " << socketState;
}

void Smtp::errorReceived(QAbstractSocket::SocketError socketError)
{
    qDebug() << "error " <<socketError;
    // something broke.
    if( state != Close)
    {
        QMessageBox::warning( nullptr, tr( "Virhe sähköpostin lähetyksessä" ), tr( "Sähköpostipalvelin ilmoitti virheen: %1" ).arg( socket->errorString())  );
        state = Close;
        emit status( tr( "Sähköpostin lähetys epäonnistui" ) );
    }
}

void Smtp::disconnected()
{

    qDebug() <<"disconneted";
}

void Smtp::connected()
{
    qDebug() << "Connected ";
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

    if ( state == Init && responseLine == "220" )
    {
        emit status( tr( "Lähetetään sähköpostia..." ) );
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
        *t << otsikko;
        t->flush();
        t->setCodec("UTF-8");
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
        emit status( tr( "Sähköposti lähetetty" ) );
    }
    else if ( state == Close )
    {
        deleteLater();
        return;
    }
    else
    {
        // something broke.        
        // QMessageBox::warning( nullptr, tr( "Virhe sähköpostin lähetyksessä" ), tr( "Sähköpostipalvelin ilmoitti virheen:\n\n" ) + response );
        state = Close;
        // emit status( tr( "Sähköpostin lähetys epäonnistui" ) );
    }
    response = "";
}

QString Smtp::poimiPelkkaOsoite(const QString osoite)
{
    QRegularExpression re("<.*@.*>");
    QRegularExpressionMatch mats = re.match(osoite);
    if( mats.hasMatch() )
        return mats.captured(0);
    return QString();
}
