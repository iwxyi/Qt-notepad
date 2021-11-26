#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class FindDialog;
}

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QSettings& settings, QWidget *parent = nullptr);
    ~FindDialog() override;

    void open(bool replace);

signals:
    void signalShow();
    void signalHide();
    // void signalTextChanged(const QString& text);
    void signalFindNext();
    void signalFindPrev();
    void signalReplaceNext();
    void signalReplaceAll();

public:
    const QString &&getFindText() const;
    const QString &&getReplaceText() const;
    bool isCaseSensitive() const;
    bool isLoopFind() const;

private slots:
    void on_findNextButton_clicked();

    void on_replaceButton_clicked();

    void on_replaceAllButton_clicked();

    void on_cancelButton_clicked();

    void on_caseSensitiveCheck_clicked();

    void on_loopCheck_clicked();

    void on_upRadio_clicked();

    void on_downRadio_clicked();

    void on_findEdit_returnPressed();

    void on_replaceEdit_returnPressed();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    Ui::FindDialog *ui;
    QSettings& settings;
};

#endif // FINDDIALOG_H
