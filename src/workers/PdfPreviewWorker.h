#pragma once

#include <QObject>
#include <QStringList>
#include <QImage>

class PdfPreviewWorker : public QObject
{
    Q_OBJECT

public:
    explicit PdfPreviewWorker(QObject* parent = nullptr);
    void setFiles(const QStringList& files);

signals:
    void pageReady(const QImage& image, const QString& label);
    void finished();

public slots:
    void process();
    void cancel();

private:
    QStringList m_files;
    bool m_cancel = false;
};