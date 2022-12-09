#ifndef KAKSIVAIHEDIALOG_H
#define KAKSIVAIHEDIALOG_H

#include <QDialog>
#include <QRegularExpression>

namespace Ui {
class KaksivaiheDialog;
}

class KaksivaiheDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KaksivaiheDialog(QWidget *parent = nullptr);
    ~KaksivaiheDialog();

    QString askCode(const QString& name);
    QString askEmailCode(const QString& email);

private:
    int exec() override;

    void edited();

    static QRegularExpression NumeroRE__;

    Ui::KaksivaiheDialog *ui;
};

#endif // KAKSIVAIHEDIALOG_H
