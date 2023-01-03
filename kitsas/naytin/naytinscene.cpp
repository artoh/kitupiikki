#include "naytinscene.h"

#include <QGraphicsSceneDragDropEvent>

#include <QDebug>
#include <QMimeData>
#include <QUrl>

namespace Naytin {

NaytinScene::NaytinScene(QObject *parent) : QGraphicsScene(parent)
{

}

void NaytinScene::salliPudotus(bool sallittu)
{
    pudotusSallittu_ = sallittu;
}

void NaytinScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if( !pudotusSallittu()) {
        return;
    }

    for(const QUrl& url : event->mimeData()->urls()) {
        if( url.isLocalFile()) {
            event->accept();
            return;
        }
    }
}

void NaytinScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    event->accept();
}

void NaytinScene::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    for(const QUrl& url : event->mimeData()->urls()) {
        if( url.isLocalFile()) {
            QString polku = url.path();

#ifdef Q_OS_WIN
                if( polku.startsWith(QChar('/')))
                    polku = polku.mid(1);
#endif
            emit tiedostoPudotettu(polku);
        }
    }
}

} // namespace Naytin
