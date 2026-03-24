#pragma once

#include <QStringList>

class PdfService {
public:
    bool mergePdfFiles(const QStringList& inputFiles, const QString& outputFile, QString& errorMessage);
};