#ifndef KPIBANEDIT_H
#define KPIBANEDIT_H

#include <QLineEdit>
#include "laskutus/iban.h"

class IbanValidator;

class KpIbanEdit : public QLineEdit
{
    Q_OBJECT
public:
    KpIbanEdit(QWidget* parent = nullptr);
    Iban iban() const;
    void setIban(const Iban& iban);

protected:
    void paintEvent(QPaintEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

    IbanValidator* validator_;
};

#endif // KPIBANEDIT_H
