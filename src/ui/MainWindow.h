#pragma once

#include <QMainWindow>

class QListWidget;
class QPushButton;
class QLineEdit;
class QStackedWidget;
class QWidget;
class QLabel;
class QPdfDocument;
class QScrollArea;
class QVBoxLayout;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addPdfFiles();
    void removeSelectedPdfFile();
    void mergePdfFiles();
    void splitPdfFile();
    void showMergePage();
    void showSplitPage();
    void showOcrPage();
    void showConvertPage();
    void updateSplitPreview();

private:
    QWidget* createMergePage();
    QWidget* createSplitPage();
    QWidget* createPlaceholderPage(const QString& title, const QString& text);

    void loadPdf(const QString& filePath);
    void clearPreview();
    void renderAllPagesPreview();
    void renderSplitPreview(const QString& pageRange);
    void renderMergePreview();

    QList<int> parsePageRange(const QString& pageRange, int maxPages) const;

    QListWidget *fileListWidget;
    QPushButton *addFilesButton;
    QPushButton *mergeButton;
    QPushButton *splitButton;
    QPushButton *ocrButton;
    QPushButton *convertButton;
    QPushButton *removeFileButton;
    QLineEdit *pageRangeEdit;

    QStackedWidget *toolStack;
    QWidget *mergePage;
    QWidget *splitPage;
    QWidget *ocrPage;
    QWidget *convertPage;

    QPdfDocument *pdfDocument;

    QScrollArea *previewScrollArea;
    QWidget *previewContainer;
    QVBoxLayout *previewLayout;
    QLabel *previewTitleLabel;
};