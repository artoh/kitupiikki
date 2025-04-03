#ifndef UUSIMAKSUDIALOG_H
#define UUSIMAKSUDIALOG_H

#include <QDialog>
#include <QDate>
#include "model/euro.h"

namespace Ui {
class UusiMaksuDlg;
}

class UusiMaksuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UusiMaksuDialog(QWidget *parent = nullptr);
    ~UusiMaksuDialog();

    void init(const QString& saaja, const QString& iban = QString(), const QString& viite = QString(), const Euro& summa = Euro::Zero, const QDate &pvm = QDate());

private:
    void updateBankName();
    void validate();

    Ui::UusiMaksuDlg *ui;
};

#endif // UUSIMAKSUDIALOG_H
