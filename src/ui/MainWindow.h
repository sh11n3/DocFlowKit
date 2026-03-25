#pragma once

#include <QMainWindow>
#include <QHash>
#include <QImage>
#include <QStringList>
#include <QVector>

class QListWidget;
class QPushButton;
class QLineEdit;
class QStackedWidget;
class QWidget;
class QLabel;
class QPdfDocument;
class QScrollArea;
class QVBoxLayout;
class QThread;
class PdfPreviewWorker;


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

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QWidget* createMergePage();
    QWidget* createSplitPage();
    QWidget* createPlaceholderPage(const QString& title, const QString& text);

    void loadPdf(const QString& filePath);
    void moveSelectedUp();
    void moveSelectedDown();
    void startMergePreviewAsync();
    void stopPreviewWorker();
    void showStatus(const QString& text);   
    void hideStatus();                      
    void clearPreview();
    void renderAllPagesPreview();
    void renderSplitPreview(const QString& pageRange);
    void renderMergePreview();
    void rebuildMergePagesFromFiles();
    void moveMergePageUp(int index);
    void moveMergePageDown(int index);

    QList<int> parsePageRange(const QString& pageRange, int maxPages) const;

    QListWidget *fileListWidget;
    QPushButton *addFilesButton;
    QPushButton *moveUpButton;
    QPushButton *moveDownButton;
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
    QLabel *statusLabel;

    QThread* previewThread = nullptr;
    PdfPreviewWorker* previewWorker = nullptr;
    QHash<QString, QList<QImage>> pdfPageCache;
    QList<QImage> getRenderedPages(const QString& filePath);
    
    struct MergePageItem {
    QString filePath;
    int pageIndex;
    };

    QVector<MergePageItem> mergePages;
};