#include "mytexteditor.h"

#include <QActionGroup>
#include <QAction>
#include <QMenu>
#include <QComboBox>
#include <QToolBar>
#include <QTextTable>
#include <QFile>

MyTextEditor::MyTextEditor(QWidget* parent) :
    QTextEdit(parent)
{
    initStyleSheets();
    createActions();
    createToolbar();
}

QToolBar *MyTextEditor::toolbar()
{
    return toolbar_;
}

void MyTextEditor::initStyleSheets()
{
    QFile styleFile(":/tilinpaatos/tilinpaatos.css");
    styleFile.open(QFile::ReadOnly);
    QString style(styleFile.readAll());
    document()->setDefaultStyleSheet(style);
    setStyleSheet("color: black; background-color: white;");

}

void MyTextEditor::createActions()
{
    undoAction_ = new QAction( QIcon(":/pic/edit-undo.png"), tr("Kumoa"), this);
    connect( document(), &QTextDocument::undoAvailable, undoAction_, &QAction::setEnabled);
    connect( undoAction_, &QAction::triggered, this, &QTextEdit::undo);
    undoAction_->setShortcut(QKeySequence::Undo);
    redoAction_ = new QAction( QIcon(":/pic/edit-redo.png"), tr("Toista"), this);
    connect( document(), &QTextDocument::redoAvailable, redoAction_, &QAction::setEnabled);
    connect( redoAction_, &QAction::triggered, this, &QTextEdit::redo);

    cutAction_ = new QAction( QIcon(":/pic/edit-cut.png"), tr("Leikkaa"), this);
    connect( cutAction_, &QAction::triggered, this, &QTextEdit::cut);
    connect( this, &QTextEdit::copyAvailable, cutAction_, &QAction::setEnabled);
    cutAction_->setShortcut(QKeySequence::Cut);
    copyAction_ = new QAction( QIcon(":/pic/edit-copy.png"), tr("Kopioi"), this);
    connect( copyAction_, &QAction::triggered, this, &QTextEdit::copy);
    connect( this, &QTextEdit::copyAvailable, copyAction_, &QAction::setEnabled);
    copyAction_->setShortcut(QKeySequence::Copy);
    pasteAction_ = new QAction( QIcon(":/pic/edit-paste.png"), tr("Liitä"), this);
        connect( pasteAction_, &QAction::triggered, this, &QTextEdit::paste);
    pasteAction_->setShortcut(QKeySequence::Paste);
    cutAction_->setEnabled(false);
    copyAction_->setEnabled(false);

    boldAction_ = new QAction( QIcon(":/pic/lihavoi.png"), tr("Lihavoi"), this);
    connect( boldAction_, &QAction::triggered, this, [this] (bool toggled) {this->setFontWeight(toggled ? QFont::Bold : QFont::Normal);} );
    boldAction_->setCheckable(true);
    italicAction_ = new QAction( QIcon(":/pic/format-italic.png"), tr("Kursivoi"), this);
    italicAction_->setCheckable(true);
    connect( italicAction_, &QAction::triggered, this, &QTextEdit::setFontItalic);
    italicAction_->setShortcut(QKeySequence::Italic);

    listAction_ = new QAction( QIcon(":/pic/format-list-unordered.png"), tr("Lista"), this);
    connect( listAction_, &QAction::triggered, this, &MyTextEditor::makeList);
    tableAction_ = new QAction( QIcon(":/pic/insert-table.png"), tr("Taulukko"), this);

    addTableActions_ = new QActionGroup(this);
    QMenu* addTableMenu = new QMenu(this);
    for(int i=2; i < 7; i++) {
        QAction* addAction = new QAction(tr("%1 saraketta").arg(i), this);
        addAction->setActionGroup(addTableActions_);
        connect( addAction, &QAction::triggered, this, [this,i] { this->makeTable(i);});
        addTableMenu->addAction(addAction);
    }
    tableAction_->setMenu(addTableMenu);

    addRowAction_ = new QAction( QIcon(":/pic/lisaarivi.png"), tr("Lisää rivi"), this);
    connect( addRowAction_, &QAction::triggered, this, &MyTextEditor::addRow);

    alignActions_ = new QActionGroup(this);
    createAlignAction(tr("Tasaa vasemmalle"), "left", Qt::AlignLeft);
    createAlignAction(tr("Tasaa keskelle"), "center", Qt::AlignHCenter);
    createAlignAction(tr("Tasaa oikealle"), "right", Qt::AlignRight);

    connect( this, &QTextEdit::currentCharFormatChanged, this, &MyTextEditor::updateCharFormat);
    connect( this, &QTextEdit::cursorPositionChanged, this, &MyTextEditor::updateParagraphFormat);

    updateParagraphFormat();
}

void MyTextEditor::createAlignAction(const QString &text, const QString &icon, Qt::Alignment align)
{
    QAction* action = new QAction(QIcon(":/pic/format-justify-" + icon + ".png"), text, this);
    action->setData((int) align);
    action->setCheckable(true);
    connect( action, &QAction::triggered, this, [this,align] { this->setAlignment(align);});   alignActions_->addAction(action);

}

void MyTextEditor::createToolbar()
{
    toolbar_ = new QToolBar(tr("Muokkaus"), this);
    toolbar_->addAction(undoAction_);
    toolbar_->addAction(redoAction_);

    toolbar_->addSeparator();

    toolbar_->addAction(cutAction_);
    toolbar_->addAction(copyAction_);
    toolbar_->addAction(pasteAction_);

    toolbar_->addSeparator();

    tekstiTyyppiCombo_ = new QComboBox();

    tekstiTyyppiCombo_->addItems( QStringList() << tr("Leipäteksti") << tr("Otsikko") << tr("Alaotsikko") << tr("Kirjoituskone"));
    toolbar_->addWidget(tekstiTyyppiCombo_);
    connect( tekstiTyyppiCombo_, &QComboBox::currentIndexChanged, this, &MyTextEditor::styleText);

    toolbar_->addSeparator();
    toolbar_->addAction( boldAction_ );
    toolbar_->addAction( italicAction_);

    toolbar_->addSeparator();
    toolbar_->addAction(listAction_);
    toolbar_->addAction(tableAction_);
    toolbar_->addAction(addRowAction_);

    toolbar_->addSeparator();
    toolbar_->addActions(alignActions_->actions());

}

void MyTextEditor::updateCharFormat(const QTextCharFormat &format)
{

    boldAction_->setChecked( format.fontWeight() >  QFont::Normal);
    italicAction_->setChecked( format.fontItalic() );
    if( format.font().family().contains("Mono")) {
        tekstiTyyppiCombo_->setCurrentIndex(Monospace);
    } else if( format.fontPointSize() > 12) {
        tekstiTyyppiCombo_->setCurrentIndex(Header);
    } else if( format.fontPointSize() > 10) {
        tekstiTyyppiCombo_->setCurrentIndex(SubHeader);
    } else {
        tekstiTyyppiCombo_->setCurrentIndex(BodyText);
    }
}

void MyTextEditor::updateParagraphFormat()
{
    const bool isTable = textCursor().currentTable();
    const bool isList = textCursor().currentList();

    listAction_->setEnabled(!isList && !isTable);
    tableAction_->setEnabled(!isList && !isTable);
    addRowAction_->setEnabled(isTable);

    const Qt::Alignment aligment = textCursor().blockFormat().alignment();
    foreach (QAction *action, alignActions_->actions())
        action->setChecked(action->data().toInt() == (int) aligment);

}

void MyTextEditor::styleText(int index)
{
    QTextCharFormat format;
    switch (index) {
    case BodyText:
        format.setFontWeight(QFont::Normal);
        format.setFontPointSize(10);
        format.setFontFamilies(QStringList() << "FreeSans");
        break;
    case Header:
        format.setFontWeight(QFont::Bold);
        format.setFontPointSize(14);
        format.setFontFamilies(QStringList() << "FreeSans");
        break;
    case SubHeader:
        format.setFontWeight(QFont::Bold);
        format.setFontPointSize(12);
        format.setFontFamilies(QStringList() << "FreeSans");
        break;
    case Monospace:
        format.setFontWeight(QFont::Normal);
        format.setFontPointSize(10);
        format.setFontFamilies(QStringList() << "FreeMono");
        break;
    default:
        break;
    }
    textCursor().setCharFormat(format);
}

void MyTextEditor::makeList()
{
    QTextCursor cursor = textCursor();
    if( !cursor.currentList()) {
        cursor.createList(QTextListFormat::ListDisc);
        updateParagraphFormat();
    }
}

void MyTextEditor::makeTable(int columns)
{
    QTextTable* table = textCursor().insertTable(2, columns);
    QTextTableFormat format = table->format();
    QList<QTextLength> cells;
    for(int i=0; i < columns; i++)
        cells << QTextLength(QTextLength::PercentageLength, 100.0 / columns);
    format.setColumnWidthConstraints(cells);
    table->setFormat(format);
}

void MyTextEditor::addRow()
{
    QTextCursor cursor = textCursor();
    if( cursor.currentTable()) {
        cursor.currentTable()->appendRows(1);
        const int lastRow = cursor.currentTable()->rows() - 1;
        for(int c=0; c < cursor.currentTable()->columns(); c++) {
            // Sama tasaus yms. kun ylemmässä solussa
            const QTextBlockFormat format = cursor.currentTable()->cellAt(lastRow-1, c).firstCursorPosition().blockFormat();
            cursor.currentTable()->cellAt(lastRow, c).firstCursorPosition().setBlockFormat(format);
        }
        for(int c=1; c < cursor.currentTable()->columns(); c++) {
            cursor.movePosition(QTextCursor::PreviousCell);
        }

    }
}

void MyTextEditor::keyPressEvent(QKeyEvent *event)
{
    if( event->key() == Qt::Key_Tab && textCursor().currentTable()) {
        if( event->modifiers() & Qt::ShiftModifier) {
            moveCursor(QTextCursor::PreviousCell);
        } else {
            const QTextTable* table = textCursor().currentTable();
            const QTextTableCell& lastCell = table->cellAt(table->rows()-1, table->columns()-1);
            if( textCursor().position() >= lastCell.firstPosition()) {
                addRow();
            } else {
                moveCursor( QTextCursor::NextCell);
            }
        }
    } else {
        QTextEdit::keyPressEvent(event);
    }
}

