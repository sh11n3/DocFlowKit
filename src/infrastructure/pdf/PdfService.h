#pragma once

#include <QString>
#include <QStringList>

class PdfService {
public:
    bool mergePdfFiles(const QStringList& inputFiles, const QString& outputFile, QString& errorMessage);
    bool splitPdfFile(const QString& inputFile, const QString& outputFile, const QString& pageRange, QString& errorMessage);
};