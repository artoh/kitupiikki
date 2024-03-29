#ifndef UUSITOIMISTODIALOG_H
#define UUSITOIMISTODIALOG_H

#include <QDialog>

namespace Ui {
class UusiToimistoDialog;
}

class GroupTreeModel;
class GroupData;

class UusiToimistoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UusiToimistoDialog(QWidget *parent = nullptr);
    ~UusiToimistoDialog();

    void newOffice(GroupTreeModel* tree, GroupData *data);
    void editOffice(GroupTreeModel* tree, GroupData *data);

private:
    void initTypes(GroupData* data);
    Ui::UusiToimistoDialog *ui;

};

#endif // UUSITOIMISTODIALOG_H
