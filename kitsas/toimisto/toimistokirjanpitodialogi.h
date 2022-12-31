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

private:
    void initUi();

    Ui::ToimistoKirjanpitoDialogi *ui;
    GroupData* groupData_;
};

#endif // TOIMISTOKIRJANPITODIALOGI_H
