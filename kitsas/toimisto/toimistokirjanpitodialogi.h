#ifndef TOIMISTOKIRJANPITODIALOGI_H
#define TOIMISTOKIRJANPITODIALOGI_H

#include <QDialog>

class GroupData;

namespace Ui {
class ToimistoKirjanpitoDialogi;
}

class ToimistoKirjanpitoDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit ToimistoKirjanpitoDialogi(QWidget *parent, GroupData* group);
    ~ToimistoKirjanpitoDialogi();

    void accept() override;    

private:
    void initUi();
    void haeTunnarilla();
    void hakuSaapuu();
    void tarkastaKelpo();

    void created();
    void error();

    Ui::ToimistoKirjanpitoDialogi *ui;
    GroupData* groupData_;
};

#endif // TOIMISTOKIRJANPITODIALOGI_H
