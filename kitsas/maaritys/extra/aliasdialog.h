#ifndef ALIASDIALOG_H
#define ALIASDIALOG_H

#include <QDialog>

namespace Ui {
class AliasDialog;
}

class AliasDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AliasDialog(QWidget *parent = nullptr);
    ~AliasDialog();

    void asetaAlias(const QString& alias);
    void tarkasta();

    void accept() override;
    QString alias() const;

protected:
    void tieto(QVariant* data);
    void tallennettu();


private:
    Ui::AliasDialog *ui;
};

#endif // ALIASDIALOG_H
