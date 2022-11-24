#ifndef RYHMAOIKEUSDIALOG_H
#define RYHMAOIKEUSDIALOG_H

#include <QDialog>
#include "groupmember.h"

namespace Ui {
class RyhmaOikeusDialog;
class OikeusWidget;
class ToimistoOikeudet;
}

class GroupData;
class BookData;

class RyhmaOikeusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RyhmaOikeusDialog(QWidget *parent, GroupData* groupData);
    ~RyhmaOikeusDialog();

    void muokkaa(const GroupMember& member, BookData *book = nullptr);
    void lisaa(BookData* book = nullptr);


    void accept() override;

protected:
    void tallennettu();
    void virhe(int koodi);

    void emailMuokattu();
    void emailLoytyy(QVariant* data);
    void emailEiLoydy(int virhe);

    void oikeusMuutos();
    void tarkasta();
    void pikaMuutos();

private:
    Ui::RyhmaOikeusDialog *ui;
    Ui::OikeusWidget *oikeusUi;
    Ui::ToimistoOikeudet *toimistoUi;             

    static QRegularExpression emailRe;
    int userId_ = 0;    
    BookData* book_ = nullptr;
    GroupData* group_ = nullptr;
};

#endif // RYHMAOIKEUSDIALOG_H
