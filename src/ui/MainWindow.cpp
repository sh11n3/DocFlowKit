#include "MainWindow.h"
#include "../infrastructure/pdf/PdfService.h"
#include "../workers/PdfPreviewWorker.h"

#include <QStackedWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QThread>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPdfDocument>
#include <QPdfPageRenderer>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <QRegularExpression>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QAbstractItemView>
#include <QListWidgetItem>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      fileListWidget(new QListWidget(this)),
      addFilesButton(new QPushButton("PDFs hinzufuegen", this)),
      removeFileButton(new QPushButton("PDF entfernen", this)),
      moveUpButton(new QPushButton("↑ Move Up", this)),
      moveDownButton(new QPushButton("↓ Move Down", this)),
      mergeButton(new QPushButton("Merge", this)),
      splitButton(new QPushButton("Split", this)),
      ocrButton(new QPushButton("OCR", this)),
      convertButton(new QPushButton("Convert", this)),
      statusLabel(new QLabel("", this)),
      pageRangeEdit(new QLineEdit(this)),
      toolStack(new QStackedWidget(this)),
      pdfDocument(new QPdfDocument(this)),
      previewScrollArea(new QScrollArea(this)),
      previewContainer(new QWidget(this)),
      previewLayout(new QVBoxLayout()),
      previewTitleLabel(new QLabel("Vorschau", this)) {

    setWindowTitle("DocFlowKit");
    resize(1300, 800);

    setAcceptDrops(true);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *rootLayout = new QHBoxLayout(central);

    auto *leftPanel = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPanel);

    mergePage = createMergePage();
    splitPage = createSplitPage();
    ocrPage = createOcrPage();
    convertPage = createPlaceholderPage("Convert", "Hier kommen spaeter Konvertierungen rein.");

    fileListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    toolStack->addWidget(mergePage);
    toolStack->addWidget(splitPage);
    toolStack->addWidget(ocrPage);
    toolStack->addWidget(convertPage);

    leftLayout->addWidget(addFilesButton);
    leftLayout->addWidget(removeFileButton);
    leftLayout->addWidget(moveUpButton);
    leftLayout->addWidget(moveDownButton);
    leftLayout->addWidget(mergeButton);
    leftLayout->addWidget(splitButton);
    leftLayout->addWidget(ocrButton);
    leftLayout->addWidget(convertButton);
    leftLayout->addSpacing(12);
    leftLayout->addWidget(new QLabel("Dateien:", this));
    leftLayout->addWidget(fileListWidget, 1);
    leftLayout->addSpacing(12);
    leftLayout->addWidget(toolStack, 1);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);

    previewTitleLabel->setAlignment(Qt::AlignCenter);

    previewContainer->setLayout(previewLayout);
    previewScrollArea->setWidget(previewContainer);
    previewScrollArea->setWidgetResizable(true);

    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: #666; font-weight: bold; padding: 6px;");
    statusLabel->hide();

    rightLayout->addWidget(previewTitleLabel);
    rightLayout->addWidget(statusLabel);
    rightLayout->addWidget(previewScrollArea, 1);

    rootLayout->addWidget(leftPanel, 0);
    rootLayout->addWidget(rightPanel, 1);

    connect(addFilesButton, &QPushButton::clicked, this, &MainWindow::addPdfFiles);
    connect(removeFileButton, &QPushButton::clicked, this, &MainWindow::removeSelectedPdfFile);
    connect(moveUpButton, &QPushButton::clicked, this, &MainWindow::moveSelectedUp);
    connect(moveDownButton, &QPushButton::clicked, this, &MainWindow::moveSelectedDown);
    connect(mergeButton, &QPushButton::clicked, this, &MainWindow::showMergePage);
    connect(splitButton, &QPushButton::clicked, this, &MainWindow::showSplitPage);
    connect(ocrButton, &QPushButton::clicked, this, &MainWindow::showOcrPage);
    connect(convertButton, &QPushButton::clicked, this, &MainWindow::showConvertPage);

    connect(fileListWidget, &QListWidget::currentTextChanged, this, [this](const QString &text) {
        if (toolStack->currentWidget() == mergePage) {
            return; // in Merge keine Neuladung bei einfachem Anklicken
        }

        loadPdf(text);
    });

    connect(pageRangeEdit, &QLineEdit::textChanged, this, &MainWindow::updateSplitPreview);
}

MainWindow::~MainWindow()
{
    stopPreviewWorker();
}

QWidget* MainWindow::createMergePage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel("Merge", this);
    title->setAlignment(Qt::AlignCenter);

    auto *info = new QLabel("Mehrere PDFs aus der Liste werden zu einer Datei zusammengefuegt.", this);
    info->setWordWrap(true);

    auto *runButton = new QPushButton("Merge ausfuehren", this);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::mergePdfFiles);

    layout->addWidget(title);
    layout->addWidget(info);
    layout->addWidget(runButton);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::createSplitPage() {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel("Split", this);
    title->setAlignment(Qt::AlignCenter);

    pageRangeEdit->setPlaceholderText("Seitenbereich, z. B. 1-3 oder 1,3,5");

    auto *runButton = new QPushButton("Split ausfuehren", this);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::splitPdfFile);

    layout->addWidget(title);
    layout->addWidget(new QLabel("Waehle eine PDF in der Liste und gib den Bereich an.", this));
    layout->addWidget(pageRangeEdit);
    layout->addWidget(runButton);
    layout->addStretch();

    return page;
}

void MainWindow::runOcrForSelectedPdf()
{
    if (fileListWidget->count() < 1 || !fileListWidget->currentItem()) {
        QMessageBox::warning(this, "Fehler", "Bitte eine PDF auswaehlen.");
        return;
    }

    QString inputFile = fileListWidget->currentItem()->text();

    QString outputFile = QFileDialog::getSaveFileName(
        this,
        "OCR PDF speichern",
        "ocr_output.pdf",
        "PDF Files (*.pdf)"
    );

    if (outputFile.isEmpty()) {
        return;
    }

    showStatus("OCR Processing...");
    QApplication::processEvents();

    PdfService pdfService;
    QString errorMessage;
    bool success = pdfService.runOcrOnPdf(inputFile, outputFile, errorMessage);

    hideStatus();

    if (success) {
        QMessageBox::information(this, "Erfolg", "Durchsuchbare PDF wurde erfolgreich erstellt.");
    } else {
        QMessageBox::critical(this, "OCR Fehler", errorMessage);
    }
}

QWidget* MainWindow::createOcrPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel("OCR", this);
    title->setAlignment(Qt::AlignCenter);

    auto *info = new QLabel("Erzeugt aus einer Bild-/Scan-PDF eine durchsuchbare PDF mit Textlayer.", this);
    info->setWordWrap(true);

    auto *runButton = new QPushButton("OCR ausfuehren", this);
    connect(runButton, &QPushButton::clicked, this, &MainWindow::runOcrForSelectedPdf);

    layout->addWidget(title);
    layout->addWidget(info);
    layout->addWidget(runButton);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::createPlaceholderPage(const QString& titleText, const QString& text) {
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *title = new QLabel(titleText, this);
    title->setAlignment(Qt::AlignCenter);

    auto *info = new QLabel(text, this);
    info->setWordWrap(true);

    layout->addWidget(title);
    layout->addWidget(info);
    layout->addStretch();

    return page;
}

void MainWindow::addPdfFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "PDF-Dateien auswaehlen",
        "",
        "PDF Files (*.pdf)"
    );

    bool added = false;

    for (const QString &file : files) {
        if (fileListWidget->findItems(file, Qt::MatchExactly).isEmpty()) {
            fileListWidget->addItem(file);
            added = true;
        }
    }

    if (fileListWidget->count() > 0 && !fileListWidget->currentItem()) {
        fileListWidget->setCurrentRow(0);
    }

    if (added && toolStack->currentWidget() == mergePage) {
        rebuildMergePagesFromFiles();
        renderMergePreview();
    }
}

void MainWindow::loadPdf(const QString& filePath) {
    if (filePath.isEmpty()) {
        return;
    }

    pdfDocument->close();
    pdfDocument->load(filePath);

    if (toolStack->currentWidget() == splitPage && !pageRangeEdit->text().trimmed().isEmpty()) {
        renderSplitPreview(pageRangeEdit->text().trimmed());
    } else {
        renderAllPagesPreview();
    }
}

void MainWindow::clearPreview() {
    QLayoutItem *item;
    while ((item = previewLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void MainWindow::renderAllPagesPreview() {
    clearPreview();

    if (pdfDocument->pageCount() <= 0) {
        previewTitleLabel->setText("Vorschau");
        return;
    }

    previewTitleLabel->setText("Original PDF");

    for (int i = 0; i < pdfDocument->pageCount(); ++i) {
        QSize pageSize = pdfDocument->pagePointSize(i).toSize();
        if (pageSize.isEmpty()) {
            continue;
        }

        QSize renderSize(600, static_cast<int>(600.0 * pageSize.height() / pageSize.width()));
        QImage image = pdfDocument->render(i, renderSize);

        QImage finalImage(image.size(), QImage::Format_RGB32);
        finalImage.fill(Qt::white);

        QPainter painter(&finalImage);
        painter.drawImage(0, 0, image);
        painter.end();

        auto *pageLabel = new QLabel(this);
        pageLabel->setAlignment(Qt::AlignCenter);
        pageLabel->setPixmap(QPixmap::fromImage(finalImage));
        pageLabel->setStyleSheet("border: 1px solid gray; margin: 8px;");

        auto *infoLabel = new QLabel(QString("Seite %1").arg(i + 1), this);
        infoLabel->setAlignment(Qt::AlignCenter);

        previewLayout->addWidget(infoLabel);
        previewLayout->addWidget(pageLabel);
    }

    previewLayout->addStretch();
}

QList<int> MainWindow::parsePageRange(const QString& pageRange, int maxPages) const {
    QList<int> result;
    QStringList parts = pageRange.split(",", Qt::SkipEmptyParts);

    for (QString part : parts) {
        part = part.trimmed();

        if (part.contains("-")) {
            QStringList bounds = part.split("-");
            if (bounds.size() != 2) {
                continue;
            }

            bool okStart = false;
            bool okEnd = false;
            int start = bounds[0].toInt(&okStart);
            int end = bounds[1].toInt(&okEnd);

            if (!okStart || !okEnd) {
                continue;
            }

            if (start > end) {
                std::swap(start, end);
            }

            for (int p = start; p <= end; ++p) {
                if (p >= 1 && p <= maxPages && !result.contains(p - 1)) {
                    result.append(p - 1);
                }
            }
        } else {
            bool ok = false;
            int page = part.toInt(&ok);
            if (ok && page >= 1 && page <= maxPages && !result.contains(page - 1)) {
                result.append(page - 1);
            }
        }
    }

    return result;
}

QList<QImage> MainWindow::getRenderedPages(const QString& filePath)
{
    if (pdfPageCache.contains(filePath)) {
        return pdfPageCache[filePath];
    }

    QList<QImage> pages;

    QPdfDocument doc;
    doc.load(filePath);

    for (int pageIndex = 0; pageIndex < doc.pageCount(); ++pageIndex) {
        QSize pageSize = doc.pagePointSize(pageIndex).toSize();
        if (pageSize.isEmpty()) continue;

        QSize renderSize(300, static_cast<int>(300.0 * pageSize.height() / pageSize.width()));
        QImage image = doc.render(pageIndex, renderSize);

        QImage finalImage(image.size(), QImage::Format_RGB32);
        finalImage.fill(Qt::white);

        QPainter painter(&finalImage);
        painter.drawImage(0, 0, image);
        painter.end();

        pages.append(finalImage);
    }

    pdfPageCache.insert(filePath, pages);
    return pages;
}

void MainWindow::renderSplitPreview(const QString& pageRange) {
    clearPreview();

    if (pdfDocument->pageCount() <= 0) {
        previewTitleLabel->setText("Split Vorschau");
        return;
    }

    QList<int> pages = parsePageRange(pageRange, pdfDocument->pageCount());

    previewTitleLabel->setText("Split Vorschau");

    if (pages.isEmpty()) {
        auto *label = new QLabel("Kein gueltiger Seitenbereich.", this);
        label->setAlignment(Qt::AlignCenter);
        previewLayout->addWidget(label);
        previewLayout->addStretch();
        return;
    }

    for (int pageIndex : pages) {
        QSize pageSize = pdfDocument->pagePointSize(pageIndex).toSize();
        if (pageSize.isEmpty()) {
            continue;
        }

        QSize renderSize(600, static_cast<int>(600.0 * pageSize.height() / pageSize.width()));
        QImage image = pdfDocument->render(pageIndex, renderSize);

        QImage finalImage(image.size(), QImage::Format_RGB32);
        finalImage.fill(Qt::white);

        QPainter painter(&finalImage);
        painter.drawImage(0, 0, image);
        painter.end();

        auto *pageLabel = new QLabel(this);
        pageLabel->setAlignment(Qt::AlignCenter);
        pageLabel->setPixmap(QPixmap::fromImage(finalImage));
        pageLabel->setStyleSheet("border: 2px solid #2d7ff9; margin: 8px;");

        auto *infoLabel = new QLabel(QString("Seite %1 (im Split enthalten)").arg(pageIndex + 1), this);
        infoLabel->setAlignment(Qt::AlignCenter);

        previewLayout->addWidget(infoLabel);
        previewLayout->addWidget(pageLabel);
    }

    previewLayout->addStretch();
}

void MainWindow::renderMergePreview()
{
    clearPreview();

    if (mergePages.isEmpty()) {
        previewTitleLabel->setText("Merge Vorschau");
        return;
    }

    previewTitleLabel->setText("Merge Vorschau");

    for (int i = 0; i < mergePages.size(); ++i) {
        const MergePageItem& item = mergePages[i];

        QPdfDocument doc;
        doc.load(item.filePath);

        QSize pageSize = doc.pagePointSize(item.pageIndex).toSize();
        if (pageSize.isEmpty()) {
            continue;
        }

        QSize renderSize(300, static_cast<int>(300.0 * pageSize.height() / pageSize.width()));
        QImage image = doc.render(item.pageIndex, renderSize);

        QImage finalImage(image.size(), QImage::Format_RGB32);
        finalImage.fill(Qt::white);

        QPainter painter(&finalImage);
        painter.drawImage(0, 0, image);
        painter.end();

        QString fileName = QFileInfo(item.filePath).fileName();

        auto *wrapper = new QWidget(this);
        auto *wrapperLayout = new QVBoxLayout(wrapper);

        auto *infoLabel = new QLabel(
            QString("%1 - Seite %2").arg(fileName).arg(item.pageIndex + 1),
            this
        );
        infoLabel->setAlignment(Qt::AlignCenter);

        auto *pageLabel = new QLabel(this);
        pageLabel->setAlignment(Qt::AlignCenter);
        pageLabel->setPixmap(QPixmap::fromImage(finalImage));
        pageLabel->setStyleSheet("border: 2px solid #3a8f3a; margin: 8px;");

        auto *buttonRow = new QWidget(this);
        auto *buttonLayout = new QHBoxLayout(buttonRow);

        auto *upButton = new QPushButton("↑", this);
        auto *downButton = new QPushButton("↓", this);

        upButton->setEnabled(i > 0);
        downButton->setEnabled(i < mergePages.size() - 1);

        connect(upButton, &QPushButton::clicked, this, [this, i]() {
            moveMergePageUp(i);
        });

        connect(downButton, &QPushButton::clicked, this, [this, i]() {
            moveMergePageDown(i);
        });

        buttonLayout->addStretch();
        buttonLayout->addWidget(upButton);
        buttonLayout->addWidget(downButton);
        buttonLayout->addStretch();

        wrapperLayout->addWidget(infoLabel);
        wrapperLayout->addWidget(pageLabel);
        wrapperLayout->addWidget(buttonRow);

        previewLayout->addWidget(wrapper);
    }

    previewLayout->addStretch();
}



void MainWindow::moveMergePageUp(int index)
{
    if (index <= 0 || index >= mergePages.size()) {
        return;
    }

    mergePages.swapItemsAt(index, index - 1);
    renderMergePreview();
}

void MainWindow::moveMergePageDown(int index)
{
    if (index < 0 || index >= mergePages.size() - 1) {
        return;
    }

    mergePages.swapItemsAt(index, index + 1);
    renderMergePreview();
}

void MainWindow::showMergePage()
{
    toolStack->setCurrentWidget(mergePage);
    rebuildMergePagesFromFiles();
    renderMergePreview();
}

void MainWindow::showStatus(const QString& text)
{
    statusLabel->setText(text);
    statusLabel->show();
}

void MainWindow::hideStatus()
{
    statusLabel->clear();
    statusLabel->hide();
}

void MainWindow::startMergePreviewAsync()
{
    stopPreviewWorker();
    clearPreview();

    QStringList files;
    for (int i = 0; i < fileListWidget->count(); ++i) {
        files << fileListWidget->item(i)->text();
    }

    previewThread = new QThread(this);
    previewWorker = new PdfPreviewWorker();

    previewWorker->setFiles(files);
    previewWorker->moveToThread(previewThread);

    connect(previewThread, &QThread::started,
            previewWorker, &PdfPreviewWorker::process);

    // 🔥 PERFORMANCE FIX (wichtig)
    connect(previewWorker, &PdfPreviewWorker::pageReady, this,
        [this](const QImage& img, const QString& label) {

            previewScrollArea->setUpdatesEnabled(false);

            auto *info = new QLabel(label, this);
            info->setAlignment(Qt::AlignCenter);

            auto *page = new QLabel(this);
            page->setPixmap(QPixmap::fromImage(img));

            previewLayout->addWidget(info);
            previewLayout->addWidget(page);

            previewScrollArea->setUpdatesEnabled(true);
        });

    connect(previewWorker, &PdfPreviewWorker::finished,
            previewThread, &QThread::quit);

    connect(previewWorker, &PdfPreviewWorker::finished,
            previewWorker, &QObject::deleteLater);

    connect(previewThread, &QThread::finished,
            previewThread, &QObject::deleteLater);


    connect(previewThread, &QThread::finished, this, [this]() {
        previewThread = nullptr;
        previewWorker = nullptr;
    });

    previewThread->start();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    for (const QUrl &url : event->mimeData()->urls()) {
        if (url.isLocalFile() && url.toLocalFile().toLower().endsWith(".pdf")) {
            event->acceptProposedAction();
            return;
        }
    }

    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    bool added = false;

    for (const QUrl &url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }

        QString filePath = url.toLocalFile();

        if (!filePath.toLower().endsWith(".pdf")) {
            continue;
        }

        if (fileListWidget->findItems(filePath, Qt::MatchExactly).isEmpty()) {
            fileListWidget->addItem(filePath);
            added = true;
        }
    }

    if (added && !fileListWidget->currentItem()) {
        fileListWidget->setCurrentRow(0);
    }

    if (added && toolStack->currentWidget() == mergePage) {
        startMergePreviewAsync();
    }

    event->acceptProposedAction();
}

void MainWindow::rebuildMergePagesFromFiles()
{
    mergePages.clear();

    for (int i = 0; i < fileListWidget->count(); ++i) {
        QString filePath = fileListWidget->item(i)->text();

        QPdfDocument doc;
        doc.load(filePath);

        for (int pageIndex = 0; pageIndex < doc.pageCount(); ++pageIndex) {
            mergePages.append({ filePath, pageIndex });
        }
    }
}

void MainWindow::stopPreviewWorker()
{
    if (previewWorker) {
        previewWorker->cancel();
    }

    if (previewThread) {
        previewThread->quit();
        previewThread->wait();

        previewThread = nullptr;
        previewWorker = nullptr;
    }
}

void MainWindow::updateSplitPreview() {
    if (toolStack->currentWidget() != splitPage) {
        return;
    }

    if (pdfDocument->pageCount() <= 0) {
        return;
    }

    QString pageRange = pageRangeEdit->text().trimmed();
    
    if (pageRange.isEmpty()) {
        renderAllPagesPreview();
    } else {
        renderSplitPreview(pageRange);
    }
}

void MainWindow::showSplitPage() {
    toolStack->setCurrentWidget(splitPage);

    if (fileListWidget->currentItem()) {
        loadPdf(fileListWidget->currentItem()->text());
    } else {
        updateSplitPreview();
    }
}

void MainWindow::showOcrPage() {
    toolStack->setCurrentWidget(ocrPage);
    renderAllPagesPreview();
}

void MainWindow::showConvertPage() {
    toolStack->setCurrentWidget(convertPage);
    renderAllPagesPreview();
}

void MainWindow::removeSelectedPdfFile()
{
    QList<QListWidgetItem*> selectedItems = fileListWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    for (QListWidgetItem* item : selectedItems) {
        delete fileListWidget->takeItem(fileListWidget->row(item));
    }

    if (fileListWidget->count() > 0) {
        fileListWidget->setCurrentRow(0);
    } else {
        stopPreviewWorker();
        pdfDocument->close();
        clearPreview();
        previewTitleLabel->setText("Vorschau");
    }

    if (toolStack->currentWidget() == mergePage) {
        rebuildMergePagesFromFiles();
        renderMergePreview();
    } else if (fileListWidget->currentItem()) {
        loadPdf(fileListWidget->currentItem()->text());
    }
}

void MainWindow::moveSelectedUp()
{
    int row = fileListWidget->currentRow();
    if (row <= 0) return;

    QListWidgetItem* item = fileListWidget->takeItem(row);
    fileListWidget->insertItem(row - 1, item);
    fileListWidget->setCurrentRow(row - 1);

    if (toolStack->currentWidget() == mergePage) {
        rebuildMergePagesFromFiles();
        renderMergePreview();
    }
}

void MainWindow::moveSelectedDown()
{
    int row = fileListWidget->currentRow();
    if (row < 0 || row >= fileListWidget->count() - 1) return;

    QListWidgetItem* item = fileListWidget->takeItem(row);
    fileListWidget->insertItem(row + 1, item);
    fileListWidget->setCurrentRow(row + 1);

    if (toolStack->currentWidget() == mergePage) {
        rebuildMergePagesFromFiles();
        renderMergePreview();
    }
}

void MainWindow::mergePdfFiles()
{
    if (mergePages.isEmpty()) {
        QMessageBox::warning(this, "Fehler", "Keine Seiten zum Export vorhanden.");
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

    QStringList pageSpecs;
    pageSpecs.reserve(mergePages.size() * 2);

    for (const MergePageItem& item : mergePages) {
        pageSpecs << item.filePath << QString::number(item.pageIndex + 1);
    }

    showStatus("Processing...");
    QApplication::processEvents();

    PdfService pdfService;
    QString errorMessage;
    bool success = pdfService.mergePdfPages(pageSpecs, outputFile, errorMessage);

    hideStatus();

    if (success) {
        QMessageBox::information(this, "Erfolg", "PDF wurde mit neuer Seitenreihenfolge erstellt.");
    } else {
        QMessageBox::critical(this, "Merge Fehler", errorMessage);
    }
}

void MainWindow::splitPdfFile() {
    if (fileListWidget->count() < 1) {
        QMessageBox::warning(this, "Fehler", "Bitte mindestens 1 PDF-Datei auswaehlen.");
        return;
    }

    QString inputFile = fileListWidget->currentItem()
        ? fileListWidget->currentItem()->text()
        : fileListWidget->item(0)->text();

    QString pageRange = pageRangeEdit->text().trimmed();
    if (pageRange.isEmpty()) {
        QMessageBox::warning(this, "Fehler", "Bitte einen Seitenbereich angeben, z. B. 1-3.");
        return;
    }

    QString outputFile = QFileDialog::getSaveFileName(
        this,
        "Split PDF speichern",
        "split.pdf",
        "PDF Files (*.pdf)"
    );

    if (outputFile.isEmpty()) {
        return;
    }

    PdfService pdfService;
    QString errorMessage;

    if (pdfService.splitPdfFile(inputFile, outputFile, pageRange, errorMessage)) {
        QMessageBox::information(this, "Erfolg", "PDF wurde erfolgreich gesplittet.");
    } else {
        QMessageBox::critical(this, "Split Fehler", errorMessage);
    }
}

