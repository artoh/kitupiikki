#ifndef VARMENNEDIALOG_H
#define VARMENNEDIALOG_H

#include <QDialog>

class GroupData;

namespace Ui {
class VarmenneDialog;
}

class VarmenneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VarmenneDialog(QWidget *parent = nullptr);
    ~VarmenneDialog();

    bool toimistoVarmenne(GroupData* group);
    int pilviVarmenne();

    QString tunnus() const;
    QString salasana() const;

protected:
    void check();

private:
    Ui::VarmenneDialog *ui;
};

#endif // VARMENNEDIALOG_H
