#include "laskurivitview.h"
#include "model/tositerivit.h"

#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QDropEvent>
#include <QDebug>
#include <QMimeData>
#include <QJsonDocument>

LaskuRivitView::LaskuRivitView(QWidget *parent) :
    QTableView(parent)
{ }

void LaskuRivitView::mousePressEvent(QMouseEvent *event)
{
    if( event->button() & Qt::LeftButton)
    {
        startPos_ = event->pos();
    }
    QTableView::mousePressEvent(event);
}

void LaskuRivitView::mouseMoveEvent(QMouseEvent *event)
{
    if( event->buttons() & Qt::LeftButton)
    {
        int distance = (event->pos() - startPos_).manhattanLength();
        QModelIndex index = indexAt(event->pos());

        if( distance >= QApplication::startDragDistance() && index.isValid() )
        {
            performDrag(index.row());
        }
    }
    QTableView::mouseMoveEvent(event);
}

void LaskuRivitView::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void LaskuRivitView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void LaskuRivitView::dropEvent(QDropEvent *event)
{
    if( event->source() == this) {
        const QModelIndex& destIndex = indexAt(event->position().toPoint());
        if( !destIndex.isValid() ) {
            return;
        }
        const int destRow = destIndex.row();
        TositeRivit* rivit = this->rivit();
        if( sourceRow_ >= 0 && sourceRow_ != destRow) {
            rivit->siirraRivi(sourceRow_, destRow);
            event->acceptProposedAction();
        }
    }
}

void LaskuRivitView::performDrag(int sourceRow)
{
    sourceRow_ = sourceRow;

    QDrag* drag = new QDrag(this);
    TositeRivit* rivit = this->rivit();

    QMimeData* mimeData = new QMimeData;
    const QVariantMap& data = rivit->rivi(sourceRow).data();
    const QJsonDocument json = QJsonDocument::fromVariant(data);
    mimeData->setData("application/json", json.toJson());

    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);

}
TositeRivit *LaskuRivitView::rivit() {
    return static_cast<TositeRivit *>(model());
}
