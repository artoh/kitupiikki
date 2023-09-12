#ifndef MYTEXTEDITOR_H
#define MYTEXTEDITOR_H

#include <QTextEdit>

class QAction;
class QActionGroup;
class QComboBox;
class QToolBar;

class MyTextEditor : public QTextEdit
{
    Q_OBJECT
public:
    MyTextEditor(QWidget *parent = nullptr);

    QToolBar* toolbar();
    void setContent(const QString& html);

protected:
    enum { BodyText, Header, SubHeader, Monospace };

    void initStyleSheets();
    void createActions();
    void createAlignAction(const QString &text, const QString &icon, Qt::Alignment align);
    void createToolbar();

    void updateCharFormat(const QTextCharFormat &format);
    void updateParagraphFormat();
    void updatePasteEnabled();

    void styleText(int index);
    void makeList();
    void makeTable(int columns);
    void addRow();

    void addMinus();

    void keyPressEvent(QKeyEvent *event) override;        

private:
    QAction *undoAction_;
    QAction *redoAction_;
    QAction *cutAction_;
    QAction *copyAction_;
    QAction *pasteAction_;
    QAction *boldAction_;
    QAction *italicAction_;

    QAction *listAction_;
    QAction *tableAction_;
    QActionGroup* addTableActions_;
    QAction *addRowAction_;
    QActionGroup* alignActions_;

    QToolBar *toolbar_;
    QComboBox *tekstiTyyppiCombo_;
};

#endif // MYTEXTEDITOR_H
