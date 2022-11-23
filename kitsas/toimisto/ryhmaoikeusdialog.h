#ifndef RYHMAOIKEUSDIALOG_H
#define RYHMAOIKEUSDIALOG_H

#include <QDialog>

namespace Ui {
class RyhmaOikeusDialog;
class OikeusWidget;
class ToimistoOikeudet;
}

class GroupData;

class RyhmaOikeusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RyhmaOikeusDialog(QWidget *parent = nullptr);
    ~RyhmaOikeusDialog();

    void lisaaRyhmaan(GroupData* group);


    void accept() override;

protected:
    void tallennettu();
    void virhe(int koodi);

    void emailMuokattu();
    void emailLoytyy(QVariant* data);
    void emailEiLoydy(int virhe);

    void tarkasta();

private:
    Ui::RyhmaOikeusDialog *ui;
    Ui::OikeusWidget *oikeusUi;
    Ui::ToimistoOikeudet *toimistoUi;             

    static QRegularExpression emailRe;
    int userId_ = 0;
    int groupId_ = 0;
    int bookId_ = 0;
};

#endif // RYHMAOIKEUSDIALOG_H
