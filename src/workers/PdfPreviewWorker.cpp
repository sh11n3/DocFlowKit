#include "PdfPreviewWorker.h"
#include <QPdfDocument>
#include <QPainter>
#include <QFileInfo>
#include <QThread>

PdfPreviewWorker::PdfPreviewWorker(QObject* parent)
    : QObject(parent) {}

void PdfPreviewWorker::setFiles(const QStringList& files) {
    m_files = files;
}

void PdfPreviewWorker::cancel() {
    m_cancel = true;
}

void PdfPreviewWorker::process()
{
    m_cancel = false;

    for (int fileIndex = 0; fileIndex < m_files.size(); ++fileIndex) {
        if (m_cancel) break;

        QPdfDocument doc;
        doc.load(m_files[fileIndex]);

        for (int pageIndex = 0; pageIndex < doc.pageCount(); ++pageIndex) {
            if (m_cancel) break;

            QSize pageSize = doc.pagePointSize(pageIndex).toSize();
            if (pageSize.isEmpty()) continue;

            QSize renderSize(300, static_cast<int>(300.0 * pageSize.height() / pageSize.width()));
            QImage image = doc.render(pageIndex, renderSize);

            QImage finalImage(image.size(), QImage::Format_RGB32);
            finalImage.fill(Qt::white);

            QPainter painter(&finalImage);
            painter.drawImage(0, 0, image);
            painter.end();

            QString fileName = QFileInfo(m_files[fileIndex]).fileName();

            QString label = QString("%1 - Seite %2")
                .arg(fileName)
                .arg(pageIndex + 1);

            emit pageReady(finalImage, label);

            QThread::msleep(5);
        }
    }

    emit finished();
}