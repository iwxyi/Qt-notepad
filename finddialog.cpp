#include "finddialog.h"
#include "ui_finddialog.h"

FindDialog::FindDialog(QSettings &settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindDialog),
    settings(settings)
{
    ui->setupUi(this);
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // 读取数据
    ui->findEdit->setText(settings.value("find/findText").toString());
    ui->replaceEdit->setText(settings.value("find/replaceText").toString());
    ui->caseSensitiveCheck->setChecked(settings.value("find/caseSensitive").toBool());
    ui->loopCheck->setChecked(settings.value("find/loop").toBool());
    if (!settings.value("find/down", true).toBool())
        ui->upRadio->setChecked(true);
}

FindDialog::~FindDialog()
{
    delete ui;
}

void FindDialog::openFind(bool replace)
{
    ui->label_3->setVisible(replace);
    ui->replaceEdit->setVisible(replace);
    ui->replaceButton->setVisible(replace);
    ui->replaceAllButton->setVisible(replace);
    ui->groupBox->setVisible(!replace);
    QDialog::show(); // open/exec会导致模态
    ui->findEdit->setFocus();
    ui->findEdit->selectAll();
    this->adjustSize();
}

const QString FindDialog::getFindText() const
{
    return ui->findEdit->text();
}

const QString FindDialog::getReplaceText() const
{
    return ui->replaceEdit->text();
}

bool FindDialog::isCaseSensitive() const
{
    return ui->caseSensitiveCheck->isChecked();
}

bool FindDialog::isLoop() const
{
    return ui->loopCheck->isChecked();
}

void FindDialog::on_findNextButton_clicked()
{
    if (ui->upRadio->isChecked())
        emit signalFindPrev();
    else
        emit signalFindNext();
    settings.setValue("find/findText", ui->findEdit->text());
}

void FindDialog::on_replaceButton_clicked()
{
    emit signalReplaceNext();
    settings.setValue("find/replaceText", ui->replaceEdit->text());
}

void FindDialog::on_replaceAllButton_clicked()
{
    emit signalReplaceAll();
    settings.setValue("find/replaceText", ui->replaceEdit->text());
}

void FindDialog::on_cancelButton_clicked()
{
    this->close();
}

void FindDialog::on_caseSensitiveCheck_clicked()
{
    settings.setValue("find/caseSensitive", ui->caseSensitiveCheck->isChecked());
}

void FindDialog::on_loopCheck_clicked()
{
    settings.setValue("find/loop", ui->loopCheck->isChecked());
}

void FindDialog::on_upRadio_clicked()
{
    settings.setValue("find/down", false);
}

void FindDialog::on_downRadio_clicked()
{
    settings.setValue("find/down", true);
}

void FindDialog::showEvent(QShowEvent *event)
{
    emit signalShow();
    adjustSize();
    QWidget::showEvent(event);
}

void FindDialog::hideEvent(QHideEvent *event)
{
    emit signalHide();
    QWidget::hideEvent(event);
}

void FindDialog::on_findEdit_returnPressed()
{
    on_findNextButton_clicked();
}

void FindDialog::on_replaceEdit_returnPressed()
{
    on_replaceButton_clicked();
}
