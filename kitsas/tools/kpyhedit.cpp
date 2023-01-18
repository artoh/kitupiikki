#include "kpyhedit.h"
#include <QRegularExpression>

#include <QKeyEvent>
#include "db/kirjanpito.h"

#include <QDebug>

KpYhEdit::KpYhEdit(QWidget *parent) :
    QLineEdit(parent),
    desimaalit_( kp()->asetukset()->luku("LaskuYksikkoDesimaalit", 2) )
{
    setAlignment(Qt::AlignRight);
    setValue(0.0);
}

void KpYhEdit::setValue(const double nValue)
{
    setText( QString::number(nValue, 'f', desimaalit_) );
}

double KpYhEdit::value() const
{
    QString arvo = QLineEdit::text();
    arvo.replace(',','.');
    arvo.replace(QRegularExpression("[\\s€]", QRegularExpression::UseUnicodePropertiesOption),"");
    return arvo.toDouble();
}

void KpYhEdit::setText(const QString &nt)
{
    int pilkunpaikka = nt.indexOf(',');
    if( pilkunpaikka == -1)
        pilkunpaikka = nt.indexOf('.');
    if( pilkunpaikka == -1)
        pilkunpaikka = nt.lastIndexOf(QRegularExpression("\\d"))+1;
    int kursorinpaikka = cursorPosition();
    int suhteellinen = pilkunpaikka - kursorinpaikka;
    int oikaistaanvalit = 0;
    for(int i=kursorinpaikka; i<pilkunpaikka;i++)
        if( nt.at(i).isSpace())
            oikaistaanvalit++;

    QString eurosa = nt.left(pilkunpaikka);
    QString sentosa = nt.mid(pilkunpaikka);
    eurosa.remove(QRegularExpression("\\s", QRegularExpression::UseUnicodePropertiesOption));
    sentosa.remove(QRegularExpression("\\D"));

    QString yhteen = eurosa + "." + sentosa;
    double arvo = yhteen.toDouble();
    setClearButtonEnabled( qAbs(arvo) > 1e-7);

    if( qAbs(arvo) < 1e-7 && !eurosa.isEmpty() && !eurosa.at(0).isDigit() && !eurosa.at(0).isSpace())
        QLineEdit::setText(QString(u8"\u2212%L1 €").arg(arvo, 0, 'f', desimaalit_));
    else
        QLineEdit::setText(QString("%L1 €").arg(arvo, 0, 'f', desimaalit_));

    int uusipilkunpaikka = text().indexOf(',');
    if( uusipilkunpaikka == -1)
        uusipilkunpaikka = text().lastIndexOf(QRegularExpression("\\d"))+1;

    int laskennallisetvalit = (suhteellinen - 1) / 3;
    if( suhteellinen < 0)
        setCursorPosition( pilkunpaikka - suhteellinen );
    else {

        int uusipaikka = uusipilkunpaikka -  suhteellinen + oikaistaanvalit - laskennallisetvalit;
        setCursorPosition(uusipaikka);
    }

}

void KpYhEdit::keyPressEvent(QKeyEvent *event)
{
    int kursorinpaikka = cursorPosition();
    const int pilkunpaikka = text().indexOf(',');

    if( kursorinpaikka == text().length()) {
        kursorinpaikka = pilkunpaikka;
    }

    if( event->key() == Qt::Key_Comma || event->key() == Qt::Key_Period) {
        if( pilkunpaikka > -1) {
            setCursorPosition(pilkunpaikka + 1);
            return;
        }
    }
    else if( event->key() == Qt::Key_Left && kursorinpaikka > 1 && text().at(kursorinpaikka-1).isSpace() )
    {
        cursorBackward(false,1);
        return;
    }
    else if( event->key() == Qt::Key_Right && kursorinpaikka < text().length()-1 && text().at(kursorinpaikka).isSpace() )
    {
        cursorForward(false,1);
        return;
    }
    else if( event->key() == Qt::Key_Backspace && kursorinpaikka == pilkunpaikka+1)
    {
        cursorBackward(false,1);
        emit textEdited( text() );
    }
    else if( event->key() == Qt::Key_Delete && kursorinpaikka == pilkunpaikka)
    {
        cursorForward(false,1);
        emit textEdited( text() );
    }
    else if( event->key() == Qt::Key_Minus) {
        // Yksikköhinta ei saa olla negatiivinen (Verkkolaskusääntö)
        return;
    }
    else if( !event->text().isEmpty() && event->text().at(0).isDigit() ) {
        if( pilkunpaikka > -1 &&
            kursorinpaikka > pilkunpaikka &&
            kursorinpaikka < text().length()) {
                setText( text().left(kursorinpaikka) +
                         event->text() +
                         text().mid(kursorinpaikka + 1));
                setCursorPosition(kursorinpaikka + 1);
                emit textEdited( text() );
                return;
        }
    }

    QLineEdit::keyPressEvent(event);
    setText( text() );
    emit textEdited( text() );

}


