#include "MainWindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      fileListWidget(new QListWidget(this)),
      addFilesButton(new QPushButton("PDFs hinzufuegen", this)),
      mergeButton(new QPushButton("Merge", this)) {

    setWindowTitle("DocFlowKit");
    resize(1000, 700);

    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QVBoxLayout(centralWidget);

    auto *titleLabel = new QLabel("DocFlowKit - PDF Merge", this);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(addFilesButton);
    buttonLayout->addWidget(mergeButton);

    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(fileListWidget);

    connect(addFilesButton, &QPushButton::clicked, this, &MainWindow::addPdfFiles);
    connect(mergeButton, &QPushButton::clicked, this, &MainWindow::mergePdfFiles);
}

MainWindow::~MainWindow() {}

void MainWindow::addPdfFiles() {
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "PDF-Dateien auswaehlen",
        "",
        "PDF Files (*.pdf)"
    );

    for (const QString &file : files) {
        if (fileListWidget->findItems(file, Qt::MatchExactly).isEmpty()) {
            fileListWidget->addItem(file);
        }
    }
}

void MainWindow::mergePdfFiles() {
    if (fileListWidget->count() < 2) {
        QMessageBox::warning(this, "Fehler", "Bitte mindestens 2 PDF-Dateien auswaehlen.");
        return;
    }

    QString outputFile = QFileDialog::getSaveFileName(
        this,
        "Merged PDF speichern",
        "merged.pdf",
        "PDF Files (*.pdf)"
    );

    if (outputFile.isEmpty()) {
        return;
    }

    QMessageBox::information(
        this,
        "Naechster Schritt",
        "UI steht. Als Naechstes bauen wir den echten Merge mit PdfService."
    );
}