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

protected:
    void updateButton();

private:
    Ui::KirjanSiirtoDialogi *ui;    
};

#endif // KIRJANSIIRTODIALOGI_H
