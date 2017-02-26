#include "onniwidget.h"

#include <QTimer>
#include <QDebug>

OnniWidget::OnniWidget(QWidget *parent) : QWidget(parent)
{
    ui = new Ui::onniWidget;
    ui->setupUi(this);

    setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute( Qt::WA_DeleteOnClose);
}

void OnniWidget::nayta(const QString &teksti, int aika)
{
    ui->viestiLabel->setText( teksti );
    show();
    QTimer::singleShot( aika, this, SLOT(close()));

}

void OnniWidget::mousePressEvent(QMouseEvent *event)
{
    hide();
    QWidget::mousePressEvent(event);
}
