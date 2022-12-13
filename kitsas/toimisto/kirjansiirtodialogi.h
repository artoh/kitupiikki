#ifndef KIRJANSIIRTODIALOGI_H
#define KIRJANSIIRTODIALOGI_H

#include <QDialog>

namespace Ui {
class KirjanSiirtoDialogi;
}

class GroupTreeModel;
class GroupData;

class KirjanSiirtoDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit KirjanSiirtoDialogi(QWidget *parent = nullptr);
    ~KirjanSiirtoDialogi();

    void siirra(int bookId, GroupTreeModel* tree, GroupData* books);

    void accept() override;

protected:
    void updateButton();
    void siirretty();

private:
    Ui::KirjanSiirtoDialogi *ui;    

    int bookId_ = 0;
    GroupData* group_ = nullptr;
};

#endif // KIRJANSIIRTODIALOGI_H
