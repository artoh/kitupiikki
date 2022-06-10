#ifndef EMAILKENTANKOROSTIN_H
#define EMAILKENTANKOROSTIN_H

#include <QSyntaxHighlighter>

class EmailKentanKorostin : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    EmailKentanKorostin(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    QStringList korostettavat_;
};

#endif // EMAILKENTANKOROSTIN_H
