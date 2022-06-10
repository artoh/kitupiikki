#include "emailkentankorostin.h"

EmailKentanKorostin::EmailKentanKorostin(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    korostettavat_ << "{{erapvm}}"
        << "{{laskunumero}}"
        << "{{yhteensa}}"
        << "{{viitenumero}}"
        << "{{iban}}"
        << "{{virtuaaliviivakoodi}}"
        << "{{yritys}}";
}

void EmailKentanKorostin::highlightBlock(const QString &text)
{
    for(int i=0; i < text.length() - 3; i++) {
        if( text.at(i) == "{" && text.at(i+1) == "{" ) {
            for( const QString& korostettava : korostettavat_) {
                if( text.mid(i, korostettava.length()).toLower() == korostettava) {
                    setFormat(i, korostettava.length(), QColor(Qt::darkGreen));
                }
            }
        }
    }
}
