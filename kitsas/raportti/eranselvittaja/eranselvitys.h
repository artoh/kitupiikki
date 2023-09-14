#ifndef ERANSELVITYS_H
#define ERANSELVITYS_H

#include <QMainWindow>
#include <QWidget>
#include <QItemSelection>
#include <QDate>

class EranSelvitysTiliModel;
class EranSelvitysEraModel;
class EranSelvitysViennit;

class QTableView;

class EranSelvitys : public QMainWindow
{
    Q_OBJECT
public:
    explicit EranSelvitys(QDate date, QWidget *parent = nullptr);
    ~EranSelvitys();

signals:

protected:
    void tiliValittu(const QItemSelection& selected);
    void eraValittu(const QItemSelection& selected);
    void naytaVienti(const QModelIndex& index);

    void eratLadattu();

    EranSelvitysTiliModel* tiliModel_;
    EranSelvitysEraModel *eraModel_;
    EranSelvitysViennit *viennit_;

    QTableView* eraView_;

    int tili_ = 0;
    QDate date_;

};

#endif // ERANSELVITYS_H
