#include "kirjanpitodelegaatti.h"

#include <QSvgRenderer>
#include <QPainter>

#include <QPalette>
#include "pilvi/badges.h"

KirjanpitoDelegaatti::KirjanpitoDelegaatti(QObject *parent, bool limitys)
    : QItemDelegate{parent}, limitys_{limitys}
{

}

void KirjanpitoDelegaatti::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect rect = option.rect;
    QRect logoRect(rect.x(), rect.y(), 32, 32);

    QRect textRect(rect.x() + 34 , rect.y(), rect.width() - 34, rect.height());

    QPixmap logo = index.data(AlustettuRooli).toBool() ?
        QPixmap::fromImage(QImage::fromData(index.data(LogoRooli).toByteArray())) :
        QPixmap(":/pic/lisaa.png");

    const QString text = index.data(Qt::DisplayRole).toString();

    QStyleOptionViewItem myOption = option;


    drawBackground(painter, option, index);
    drawDecoration(painter, option, logoRect, logo);

    painter->save();
    if( index.data(HarjoitusRooli).toBool()) {
        if( QPalette().base().color().lightness() > 128)
            painter->setPen(QColor(Qt::darkGreen));
        else
            painter->setPen(QColor(Qt::green));
    } else {
        painter->setPen(QPen(QPalette().text(),1));
    }

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter ,text);

    painter->restore();
    int indeksi = 1;
    int badges = index.data(BadgesRooli).toInt();

    if( badges & Badges::MARKED) {
        QSvgRenderer renderer(QString(":/pic/huomio.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }
    if( badges & Badges::DRAFT) {
        QSvgRenderer renderer(QString(":/pic/draft.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }
    if( badges & Badges::OUTBOX) {
        QSvgRenderer renderer(QString(":/pic/paperilennokki.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }
    if( badges & Badges::INBOX) {
        QSvgRenderer renderer(QString(":/pic/kierto.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }
    if( badges & Badges::NOTIFICATION ) {
        QSvgRenderer renderer(QString(":/pic/ilmoitus-vihrea.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }

    if( badges & Badges::INFORMATION ) {
        QSvgRenderer renderer(QString(":/pic/ilmoitus-keltainen.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }

    if( badges & Badges::WARN ) {
        QSvgRenderer renderer(QString(":/pic/ilmoitus-oranssi.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }

    if( badges & Badges::ERROR ) {
        QSvgRenderer renderer(QString(":/pic/ilmoitus-punainen.svg"));
        renderer.render(painter, oikealla(indeksi++, rect));
    }


    drawFocus(painter, option, rect);
}

QSize KirjanpitoDelegaatti::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize base = QItemDelegate::sizeHint(option, index);

    return QSize(base.width(), base.height() > 34 ? base.height() : 34);
}

QRect KirjanpitoDelegaatti::oikealla(int indeksi, QRect iso) const
{
    const int marginaali = iso.height() / 10;
    const int bkoko = iso.height() * 8 / 10;

    const int x = limitys_ ?
        iso.x() + iso.width() - iso.height() - (indeksi > 1 ? (indeksi - 1) * (bkoko / 3 * 2) : 0)    :
        iso.x() + iso.width() - indeksi * bkoko - marginaali;

    return QRect( x, iso.y() + marginaali,
                 bkoko, bkoko);
}
