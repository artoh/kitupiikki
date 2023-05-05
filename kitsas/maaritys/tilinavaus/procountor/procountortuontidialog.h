#ifndef PROCOUNTORTUONTIDIALOG_H
#define PROCOUNTORTUONTIDIALOG_H

#include <QDialog>

#include "procountortuontitiedosto.h"
#include "../tilinavausmodel.h"

namespace Ui {
class ProcountorTuontiDialog;
}

class ProcountorTuontiDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcountorTuontiDialog(QWidget *parent, TilinavausModel* tilinavaus);
    ~ProcountorTuontiDialog();

protected:
    void avaaTiedosto();
    bool tuoTiedosto(const QString& tiedostonnimi);
    bool onkoJo(const ProcountorTuontiTiedosto& t);

    void paivitaRuksit();

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void accept() override;


private:
    Ui::ProcountorTuontiDialog *ui;

    TilinavausModel *tilinavaus_;

    ProcountorTuontiTiedosto taseEdellinen_;
    ProcountorTuontiTiedosto tulosEdellinen_;
    ProcountorTuontiTiedosto taseNykyinen_;
    ProcountorTuontiTiedosto tulosNykyinen_;
};

#endif // PROCOUNTORTUONTIDIALOG_H
