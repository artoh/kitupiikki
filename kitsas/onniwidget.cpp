#include "onniwidget.h"

#include <QTimer>
#include <QDebug>
#include <QIcon>

OnniWidget::OnniWidget(QWidget *parent) : QWidget(parent)
{
    ui = new Ui::onniWidget;
    ui->setupUi(this);
    hide();

    QFont font;
    font.setPointSize( font.pointSize() * 3 / 2 );
    ui->viestiLabel->setFont(font);


    setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
}

void OnniWidget::nayta(const QString &teksti, Kirjanpito::Onni tyyppi, int aika)
{
    if( tyyppi == Kirjanpito::Onnistui) {
        setStyleSheet("background-color: rgba(0, 255, 0, 80)");
        ui->kuvaLabel->setPixmap(QPixmap(":/pic/ok.png"));
    } else if( tyyppi == Kirjanpito::Verkkovirhe) {
        setStyleSheet("background-color: rgba(255, 255, 0, 125)");
        ui->kuvaLabel->setPixmap(QPixmap(":/pic/verkkovirhe.png"));
    } else if( tyyppi == Kirjanpito::Stop) {
        setStyleSheet("background-color: rgba(255, 0, 0, 125)");
        ui->kuvaLabel->setPixmap(QPixmap(":/pic/stop.png"));
    } else if( tyyppi == Kirjanpito::Haetaan) {
        setStyleSheet("background-color: rgba(255, 255, 100, 125)");
        ui->kuvaLabel->setPixmap(QPixmap(":/pic/tiedostoon.png"));
    } else if( tyyppi == Kirjanpito::Onni::Ladattu) {
        setStyleSheet("background-color: rgba(152, 255, 0, 80)");
        ui->kuvaLabel->setPixmap(QPixmap(":/pic/paivita.png"));
    }

    ui->viestiLabel->setText( teksti );
    show();    
    adjustSize();
    raise();
    ikkunat++;
    QTimer::singleShot( aika, this, SLOT(aikakului()));

}

void OnniWidget::aikakului()
{
    ikkunat--;
    if(ikkunat < 1)
        hide();
}

void OnniWidget::mousePressEvent(QMouseEvent *event)
{
    hide();
    QWidget::mousePressEvent(event);
}
