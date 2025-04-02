#include "kpibanedit.h"

#include "validator/ibanvalidator.h"

#include <QPainter>

KpIbanEdit::KpIbanEdit(QWidget *parent) :
    QLineEdit{parent},
    validator_{new IbanValidator(this)}
{}

Iban KpIbanEdit::iban() const
{
    return Iban( text() );
}

void KpIbanEdit::setIban(const Iban &iban)
{
    setText( iban.valeilla() );
}

void KpIbanEdit::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    QPainter painter(this);

    if( !text().isEmpty() ) {
        if( validator_->kelpaako(text())) {
            painter.drawPixmap( width()-20, height() / 2 - 8,  16, 16, QPixmap(":/pic/ok.png") );
        } else {
            painter.drawPixmap( width()-20, height() / 2 - 8,  16, 16, QPixmap(":/pic/varoitus.png") );
        }
    }
}

void KpIbanEdit::focusOutEvent(QFocusEvent *event)
{
    Iban iban( text());
    if( iban.isValid()) {
        const QString& formatted = iban.valeilla();
        if( formatted != text()) {
            setText( formatted );
        }
    }
    QLineEdit::focusOutEvent(event);
}


