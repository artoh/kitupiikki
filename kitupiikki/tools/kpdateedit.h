/*
   Copyright (C) 2018 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KPDATEEDIT_H
#define KPDATEEDIT_H

#include <QLineEdit>
#include <QDate>



class QPushButton;
class QCalendarWidget;

/**
 * @brief Päivämääräeditori Kitupiikin tarpeisiin
 *
 * @author Arto Hyvättinen
 * @since 0.12
 *
 * Kirjanpidon päivämäärien syöttöwidget. Osittain yhteensopiva QDateEditin kanssa.
 *
 */
class KpDateEdit : public QLineEdit
{
    Q_OBJECT
public:
    KpDateEdit(QWidget *parent = nullptr);
    ~KpDateEdit();

    QSize sizeHint() const override;

    /**
     * @brief Nykyinen päivä
     * @return
     */
    QDate date() const { return date_; }

    /**
     * @brief Sallittu päivämääräväli
     * @param min Pienin sallittu pvm
     * @param max Suurin sallittu pvm
     */
    void setDateRange(const QDate& min, const QDate& max);

    /**
     * @brief Pienin sallittu pvm
     * @return
     */
    QDate minimumDate() const { return minDate_; }

    /**
     * @brief Suurin sallittu pvm
     * @return
     */
    QDate maximumDate() const { return maxDate_; }

    void setCalendarPopup(bool enable);

    bool calendarPopup() const { return popupKaytossa_; }

signals:
    /**
     * @brief Ilmoittaa valitun päivämäärän muuttuneen
     * @param date
     */
    void dateChanged(const QDate& date);

    /**
     * @brief Muokkaus tullut valmiiksi
     */
//    void editingFinished();

public slots:
    /**
     * @brief Näyttää kalenterin
     */
    void kalenteri();

    /**
     * @brief Asettaa päivämäärän
     * @param date
     */
    void setDate(QDate date);

protected slots:
    void editMuuttui(const QString &uusi);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    QCalendarWidget *kalenteri_;

    QDate date_;
    QDate minDate_;
    QDate maxDate_;
    bool popupKaytossa_;
    quint64 suljettu_;

};

#endif // KPDATEEDIT_H
