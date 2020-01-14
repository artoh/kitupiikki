/*
Copyright (c) 2013 Raivis Strogonovs
http://morf.lv
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/

#ifndef SMTP_H
#define SMTP_H


#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QSslSocket>
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <QtWidgets/QMessageBox>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>


/**
 * @brief Smtp-viestin lähetys (SSH)
 *
 * @author 2013 Raivis Strogonovs
 *
 * Muutokset: Portissa 25 ilman suojausta, osoite muodossa "Nimi" osoite@domain, plaintext/utf-8 muodossa
 *
 */
class Smtp : public QObject
{
    Q_OBJECT


public:
    Smtp( const QString &user, const QString &pass,
          const QString &host, int port = 465, int timeout = 30000 );
    ~Smtp();

    void lahetaLiitteella(const QString& from, const QString &to,
                          const QString& subject, const QString& viesti,
                          const QString& liitenimi, const QByteArray& liite);

    enum SmtpStatus {
        Unconnected,
        Connecting,
        Sending,
        Send,
        Failed
    };

signals:
    void status( SmtpStatus status);

private slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void errorReceived(QAbstractSocket::SocketError socketError);
    void disconnected();
    void connected();
    void readyRead();

private:
    int timeout;
    QString message;
    QTextStream *t;
    QSslSocket *socket;
    QString from;
    QString rcpt;
    QString response;
    QString user;
    QString pass;
    QString host;
    int port;
    enum states{Tls, HandShake ,Auth,User,Pass,Rcpt,Mail,Data,Init,Body,Quit,Close};
    int state;

    /**
     * @brief Poimii pelkän osoitteen saajasta
     * @param osoite Saaja muodossa "Nimi" <osoite@domain>
     * @return pelkkä osoite muodossa <osoite@domain>
     */
    QString poimiPelkkaOsoite(const QString osoite);
};
#endif
