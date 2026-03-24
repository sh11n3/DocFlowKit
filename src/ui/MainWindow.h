#pragma once

#include <QMainWindow>

class QListWidget;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addPdfFiles();
    void mergePdfFiles();

private:
    QListWidget *fileListWidget;
    QPushButton *addFilesButton;
    QPushButton *mergeButton;
};