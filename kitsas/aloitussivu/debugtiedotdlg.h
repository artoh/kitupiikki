#ifndef DEBUGTIEDOTDLG_H
#define DEBUGTIEDOTDLG_H

#include <QDialog>

namespace Ui {
class DebugTiedotDlg;
}
class JarjestelmaTiedot;

class DebugTiedotDlg : public QDialog
{
    Q_OBJECT

public:
    explicit DebugTiedotDlg(QWidget *parent = nullptr);
    ~DebugTiedotDlg();

    void accept() override;

protected:
    QVariantMap data();
    void sent();
    void virhe();

private:
    Ui::DebugTiedotDlg *ui;
    JarjestelmaTiedot *jarjestelma;
};

#endif // DEBUGTIEDOTDLG_H
