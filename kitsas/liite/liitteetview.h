#ifndef LIITTEETVIEW_H
#define LIITTEETVIEW_H

#include <QListView>

class LiitteetModel;

class LiitteetView : public QListView
{
    Q_OBJECT
public:
    LiitteetView(QWidget *parent = nullptr);

    void setLiitteetModel(LiitteetModel* model);

protected:
    void valitse(int indeksi);
    void valintaMuuttui(const QModelIndex& uusi);

    LiitteetModel* model_;
};

#endif // LIITTEETVIEW_H
