#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QProcess>
#include <QDesktopServices>
#include <QDateTime>
#include <QFontDialog>
#include <QTextBlock>
#include <QFileIconProvider>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),
      settings("MyNotepad")
{
    ui->setupUi(this);

    // 读取设置
    if (!settings.value("wordWrap", true).toBool())
    {
        ui->actionWord_Wrap_W->setChecked(false);
        ui->plainTextEdit->setWordWrapMode(QTextOption::NoWrap);
    }
    if (!settings.value("statusBar", true).toBool())
    {
        this->statusBar()->hide();
        ui->actionStatus_Bar_S->setChecked(false);
    }

    // 恢复字体
    QString fs;
    if (!(fs = settings.value("font").toString()).isEmpty())
    {
        QFont f;
        f.fromString(fs);
        ui->plainTextEdit->setFont(f);
    }

    // 状态栏
    posLabel = new QLabel("第 1 行，第 1 列", this);
    zoomLabel = new QLabel("100%", this);
    lineLabel = new QLabel("Windows (CRLF)", this);
    codecLabel = new QLabel("GBK", this);
    ui->statusbar->addPermanentWidget(new QLabel(this), 6);
    ui->statusbar->addPermanentWidget(posLabel, 3);
    ui->statusbar->addPermanentWidget(zoomLabel, 1);
    ui->statusbar->addPermanentWidget(lineLabel, 3);
    ui->statusbar->addPermanentWidget(codecLabel, 1);

    // 设置为系统notepad图标
    QFileIconProvider ip;
    QIcon icon = ip.icon(QFileInfo("C:\\Windows\\System32\\notepad.exe"));
    qApp->setWindowIcon(icon);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile(QString path)
{
    filePath = path;

    if (path.isEmpty())
    {
        savedContent = "";
        fileName = "无标题";
    }
    else
    {
        QFile file(path);
        if (!file.exists())
        {
            qWarning() << "文件不存在";
            return ;
        }
        fileName = QFileInfo(path).baseName();

        // 读取文件
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "打开文件失败";
            return ;
        }
        savedContent = QString::fromLocal8Bit(file.readAll());
    }
    ui->plainTextEdit->setPlainText(savedContent);
    updateWindowTitle();
}

bool MainWindow::isModified() const
{
    return ui->plainTextEdit->toPlainText() != savedContent;
}

/**
 * @brief MainWindow::askSave
 * @return 是否继续
 */
bool MainWindow::askSave()
{
    if (!isModified())
        return true;

    // 有未保存的更改
    int btn = QMessageBox::question(this, "记事本", "你想更改保存到 " + (fileName.isEmpty() ? "无标题" : fileName) + "吗？", "保存(&S)", "不保存(&N)", "取消");
    if (btn == 2) // 取消
        return false;
    if (btn == 0) // 保存
    {
        return on_actionSave_triggered();
    }
    return true;
}

void MainWindow::updateWindowTitle()
{
    this->setWindowTitle((isModified() ? "*" : "") + fileName + " - 记事本");
}

void MainWindow::createFindDialog()
{
    findDialog = new FindDialog(settings, this);

    connect(findDialog, &FindDialog::signalShow, this, [=]{
        ui->actionFind_Next_N->setEnabled(true);
        ui->actionFind_Prev_V->setEnabled(true);
    });
    connect(findDialog, &FindDialog::signalHide, this, [=]{
        ui->actionFind_Next_N->setEnabled(false);
        ui->actionFind_Prev_V->setEnabled(false);
    });
    /* connect(findDialog, &FindDialog::signalTextChanged, this, [=](const QString& text){
        this->findText = text;
    }); */
    connect(findDialog, &FindDialog::signalFindNext, this, &MainWindow::on_actionFind_Next_N_triggered);
    connect(findDialog, &FindDialog::signalFindPrev, this, &MainWindow::on_actionFind_Prev_V_triggered);
    connect(findDialog, &FindDialog::signalReplaceNext, this, [=]{
        const QString& findText = findDialog->getFindText();
        const QString& replaceText = findDialog->getReplaceText();
        if (findText.isEmpty())
            return ;

        // 替换 逻辑上分为：查找、选中、替换
        const QString& selectedText = ui->plainTextEdit->textCursor().selectedText();
        if ((findDialog->isCaseSensitive() && selectedText != findText)
                || selectedText.toLower() != findText.toLower())
        {
            // 如果选中的词不是findText，则查找下一个
            on_actionFind_Next_N_triggered();
            qInfo() << "查找：" << findText;
        }
        else
        {
            // 已选中，则替换选中的
            QTextCursor tc = ui->plainTextEdit->textCursor();
            tc.insertText(replaceText);
            ui->plainTextEdit->setTextCursor(tc);
            qInfo() << "替换：" << findText << "->" << replaceText;
            on_actionFind_Next_N_triggered(); // 查找下一个
        }
    });
    connect(findDialog, &FindDialog::signalReplaceAll, this, [=]{
        const QString& findText = findDialog->getFindText();
        const QString& replaceText = findDialog->getReplaceText();
        if (findText.isEmpty())
            return ;
        qInfo() << "全部替换：" << findText << "->" << replaceText;

        QString content = ui->plainTextEdit->toPlainText();
        QTextCursor tc = ui->plainTextEdit->textCursor();
        tc.setPosition(0);
        tc.setPosition(content.length(), QTextCursor::KeepAnchor);
        content.replace(findText, replaceText);
        tc.insertText(content);
        // ui->plainTextEdit->setTextCursor(tc);     // 不调用这句话，保留替换之前的位置
        // ui->plainTextEdit->setPlainText(content); // 这个会导致无法撤销，而且会重置光标位置到开头
    });
}

void MainWindow::showEvent(QShowEvent *e)
{
    this->restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    this->restoreState(settings.value("mainwindow/state").toByteArray());

    QMainWindow::showEvent(e);
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (!askSave())
    {
        e->ignore();
        return ;
    }
    settings.setValue("mainwindow/geometry", this->saveGeometry());
    settings.setValue("mainwindow/state", this->saveState());

    QMainWindow::closeEvent(e);
}

void MainWindow::on_plainTextEdit_textChanged()
{
    if (fileName.isEmpty())
        fileName = "无标题";
    updateWindowTitle();

    bool empty = ui->plainTextEdit->toPlainText().isEmpty();
    ui->actionFind_F->setEnabled(!empty);
    ui->actionReplace_R->setEnabled(!empty);
    ui->actionFind_Next_N->setEnabled(!empty && findDialog && findDialog->isVisible());
    ui->actionFind_Prev_V->setEnabled(!empty && findDialog && findDialog->isVisible());
}

void MainWindow::on_plainTextEdit_cursorPositionChanged()
{
    QTextCursor tc = ui->plainTextEdit->textCursor();

    QTextLayout* ly = tc.block().layout();
    int posInBlock = tc.position() - tc.block().position(); // 当前光标在block内的相对位置
    int line = ly->lineForTextPosition(posInBlock).lineNumber() + tc.block().firstLineNumber();

    int col = tc.columnNumber(); // 第几列
    // int row = tc.blockNumber(); // 第几段，无法识别WordWrap的第几行
    posLabel->setText("第 " + QString::number(line + 1) + " 行，第 " + QString::number(col + 1) + " 列");
}

void MainWindow::on_plainTextEdit_undoAvailable(bool b)
{
    ui->actionUndo_U->setEnabled(b);
}

void MainWindow::on_plainTextEdit_selectionChanged()
{
    bool selected = ui->plainTextEdit->textCursor().hasSelection();
    ui->actionSearch_By_Bing->setEnabled(selected);
    ui->actionCut_T->setEnabled(selected);
    ui->actionCopy_C->setEnabled(selected);
    ui->actionDelete_L->setEnabled(selected);
    ui->actionReselect_Chinese->setEnabled(selected);
}

void MainWindow::on_actionNew_triggered()
{
    if (!askSave())
        return ;

    openFile("");
}

void MainWindow::on_actionNew_Window_triggered()
{
    QProcess p(this);
    p.startDetached(QApplication::applicationFilePath(), {"-new"});
}

void MainWindow::on_actionOpen_triggered()
{
    if (!askSave())
        return ;

    QString recentPath = settings.value("recent/filePath").toString();
    QString path = QFileDialog::getOpenFileName(this, "打开", recentPath, "*.txt");
    if (path.isEmpty())
        return ;

    openFile(path);
}

bool MainWindow::on_actionSave_triggered()
{
    if (filePath.isEmpty()) // 没有路径，另存为
    {
        QString recentPath = settings.value("recent/filePath").toString();
        QString path = QFileDialog::getSaveFileName(this, "另存为", recentPath, "*.txt");
        if (path.isEmpty())
            return false;
        settings.setValue("recent/filePath", path);
        filePath = path;
        fileName = QFileInfo(path).baseName();
    }

    // 写出文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "打开文件失败";
        return false;
    }
    QTextStream ts(&file);
    ts.setCodec("GBK");
    savedContent = ui->plainTextEdit->toPlainText();
    ts << savedContent;
    file.close();
    qInfo() << "save:" << filePath << savedContent.length();
    updateWindowTitle();
    return true;
}

bool MainWindow::on_actionSave_As_triggered()
{
    QString temp = filePath;
    filePath = "";
    if (!on_actionSave_triggered()) // 直接调用保存的
        filePath = temp;
    return true;
}

void MainWindow::on_actionExit_triggered()
{
    if (!askSave())
        return ;

    this->close();
}

void MainWindow::on_actionUndo_U_triggered()
{
    ui->plainTextEdit->undo();
}

void MainWindow::on_actionCut_T_triggered()
{
    ui->plainTextEdit->cut();
}

void MainWindow::on_actionCopy_C_triggered()
{
    ui->plainTextEdit->copy();
}

void MainWindow::on_actionPaste_P_triggered()
{
    ui->plainTextEdit->paste();
}

void MainWindow::on_actionDelete_L_triggered()
{
    QTextCursor tc = ui->plainTextEdit->textCursor();
    int pos = tc.position();
    if (pos >= ui->plainTextEdit->toPlainText().length())
        return ;
    tc.setPosition(pos + 1, QTextCursor::MoveMode::KeepAnchor);
    tc.removeSelectedText();
}

void MainWindow::on_actionSearch_By_Bing_triggered()
{
    // !这里是转到UTF-8，保存到txt是保存为GBK
    QByteArray key = ui->plainTextEdit->textCursor().selectedText().toUtf8().toPercentEncoding();
    QDesktopServices::openUrl(QUrl("https://cn.bing.com/search?q=" + key + "&form=NPCTXT"));
}


void MainWindow::on_actionSelect_All_A_triggered()
{
    ui->plainTextEdit->selectAll();
}

void MainWindow::on_actionTime_Date_D_triggered()
{
    ui->plainTextEdit->insertPlainText(QDateTime::currentDateTime().toString("hh:mm yyyy/MM/dd"));
}

void MainWindow::on_actionWord_Wrap_W_triggered()
{
    if (ui->plainTextEdit->wordWrapMode() == QTextOption::NoWrap)
    {
        ui->plainTextEdit->setWordWrapMode(QTextOption::WordWrap);
        ui->actionWord_Wrap_W->setChecked(true);
        settings.setValue("wordWrap", true);
    }
    else
    {
        ui->plainTextEdit->setWordWrapMode(QTextOption::NoWrap);
        ui->actionWord_Wrap_W->setChecked(false);
        settings.setValue("wordWrap", false);
    }
}

void MainWindow::on_actionFont_F_triggered()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, ui->plainTextEdit->font(), this, "字体");
    if (!ok)
        return ;

    ui->plainTextEdit->setFont(f);
    settings.setValue("font", f.toString());
}

void MainWindow::on_actionZoom_In_I_triggered()
{
    if (zoomSize >= 500)
        return ;

    ui->plainTextEdit->zoomIn(1);
    zoomSize += 10;
    zoomLabel->setText(QString::number(zoomSize) + "%");
}

void MainWindow::on_actionZoom_Out_O_triggered()
{
    if (zoomSize <= 10)
        return ;

    ui->plainTextEdit->zoomOut(1);
    zoomSize -= 10;
    zoomLabel->setText(QString::number(zoomSize) + "%");
}

void MainWindow::on_actionZoom_Default_triggered()
{
    QString fs;
    if (!(fs = settings.value("font").toString()).isEmpty())
    {
        QFont f;
        f.fromString(fs);
        ui->plainTextEdit->setFont(f);
    }
    else
    {
        ui->plainTextEdit->setFont(qApp->font());
    }

    zoomSize = 100;
    zoomLabel->setText(QString::number(zoomSize) + "%");
}

void MainWindow::on_actionStatus_Bar_S_triggered()
{
    if (this->statusBar()->isHidden())
    {
        this->statusBar()->show();
        ui->actionStatus_Bar_S->setChecked(true);
        settings.setValue("statusBar", true);
    }
    else
    {
        this->statusBar()->hide();
        ui->actionStatus_Bar_S->setChecked(false);
        settings.setValue("statusBar", false);
    }
}

void MainWindow::on_actionAbout_A_triggered()
{
    QMessageBox::about(this, "关于", "高仿 Windows 记事本的 Qt 实现方案");
}

void MainWindow::on_actionFind_F_triggered()
{
    if (!findDialog)
    {
        createFindDialog();
    }
    findDialog->openFind(false);
}

void MainWindow::on_actionFind_Next_N_triggered()
{
    const QString& text = findDialog->getFindText();
    if (text.isEmpty())
        return ;

    QTextDocument::FindFlags flags;
    if (findDialog->isCaseSensitive())
        flags |= QTextDocument::FindCaseSensitively;
    bool rst = ui->plainTextEdit->find(text, flags);
    if (!rst && findDialog->isLoop()
            && ui->plainTextEdit->toPlainText().contains(text)) // 没找到，尝试从头开始
    {
        qInfo() << "从开头查找";
        QTextCursor tc = ui->plainTextEdit->textCursor();
        tc.setPosition(0);
        ui->plainTextEdit->setTextCursor(tc);
        on_actionFind_Next_N_triggered();
    }
}

void MainWindow::on_actionFind_Prev_V_triggered()
{
    const QString& text = findDialog->getFindText();
    if (text.isEmpty())
        return ;

    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (findDialog->isCaseSensitive())
        flags |= QTextDocument::FindCaseSensitively;
    bool rst = ui->plainTextEdit->find(text, flags);
    if (!rst && findDialog->isLoop()
            && ui->plainTextEdit->toPlainText().contains(text))
    {
        qInfo() << "从末尾查找";
        QTextCursor tc = ui->plainTextEdit->textCursor();
        tc.setPosition(ui->plainTextEdit->toPlainText().length());
        ui->plainTextEdit->setTextCursor(tc);
        on_actionFind_Prev_V_triggered();
    }
}

void MainWindow::on_actionReplace_R_triggered()
{
    if (!findDialog)
    {
        createFindDialog();
    }
    findDialog->openFind(true);
}

void MainWindow::on_actionGoto_G_triggered()
{
    // TODO:转到
    // 我的记事本这个选项一直是灰色的
}

void MainWindow::on_actionHelp_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/iwxyi/Qt-notepad"));
}

void MainWindow::on_actionFeedback_F_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/iwxyi/Qt-notepad/issues"));
}

void MainWindow::on_plainTextEdit_customContextMenuRequested(const QPoint&)
{
    QMenu* menu = new QMenu();

    QMenu* insertUnicodeControlCharsMenu = new QMenu("插入 Unicode 控制字符(&I)", menu);
    QList<QPair<QString, QString>> unicodeControlChars{
        {"LRM", "&Left-to-right mark"},
        {"RLM", "&Right-to-left mark"},
        {"ZWJ", "Zero width joiner"},
        {"ZWNJ", "Zero width &non-joiner"},
        {"LRE", "Start of left-to-right &embedding"},
        {"RLE", "Start of right-to-left e&mbedding"},
        {"LRO", "Start of left-to-right &override"},
        {"RLO", "Start of right-to-left o&verride"},
        {"PDF", "&Pop directional formatting"},
        {"NADS", "N&ational digit shapes substitution"},
        {"NODS", "Nominal (European) &digit shapes"},
        {"ASS", "Activate &symmetric swapping"},
        {"ISS", "Inhibit s&ymmetric swapping"},
        {"AAFS", "Activate Arabic &form shaping"},
        {"IAFS", "Inhibit Arabic form s&haping"},
        {"RS", "Record Separator (&Block separator)"},
        {"US", "Unit Separator (&Segment separator)"}
    };
    for (auto p: unicodeControlChars)
    {
        QAction* action = new QAction(p.first, insertUnicodeControlCharsMenu);
        action->setToolTip(p.second);
        // TODO: 把提示也显示出来
        connect(action, &QAction::triggered, ui->plainTextEdit, [=]{
            // TODO: 插入 Unicode 控制字符
            // ui->plainTextEdit->insertPlainText("\u202c");
        });
        insertUnicodeControlCharsMenu->addAction(action);
    }

    menu->addAction(ui->actionUndo_U);
    menu->addSeparator();
    menu->addAction(ui->actionCut_T);
    menu->addAction(ui->actionCopy_C);
    menu->addAction(ui->actionPaste_P);
    menu->addAction(ui->actionDelete_L);
    menu->addSeparator();
    menu->addAction(ui->actionSelect_All_A);
    menu->addSeparator();
    menu->addAction(ui->actionRead_Direction);
    menu->addAction(ui->actionShow_Unicode_Control_Chars);
    menu->addMenu(insertUnicodeControlCharsMenu);
    menu->addSeparator();
    menu->addAction(ui->actionRead_Mode);
    menu->addAction(ui->actionReselect_Chinese);
    menu->addSeparator();
    menu->addAction(ui->actionSearch_By_Bing);

    menu->exec(QCursor::pos());
    menu->deleteLater();
}

void MainWindow::on_actionRead_Direction_triggered()
{
    auto direction = ui->actionRead_Direction->isChecked() ? Qt::RightToLeft : Qt::LeftToRight;
    ui->plainTextEdit->setLayoutDirection(direction);
}

void MainWindow::on_actionRead_Mode_triggered()
{
    if (ui->plainTextEdit->isReadOnly())
    {
        ui->plainTextEdit->setReadOnly(false);
        ui->actionRead_Mode->setText("关闭输入法(&L)");
    }
    else
    {
        ui->plainTextEdit->setReadOnly(true);
        ui->actionRead_Mode->setText("打开输入法(&O)");
    }
}

void MainWindow::on_actionShow_Unicode_Control_Chars_triggered()
{
    // TODO: 显示 Unicode 控制字符
}

void MainWindow::on_actionReselect_Chinese_triggered()
{
    // TODO: 汉字重选
}

void MainWindow::on_actionPrefrence_triggered()
{
    // TODO: 页面设置
}

void MainWindow::on_actionPrint_triggered()
{
    // TODO: 打印
}
