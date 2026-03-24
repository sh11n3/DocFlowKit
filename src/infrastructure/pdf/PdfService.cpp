#include "PdfService.h"

#include <QProcess>

bool PdfService::mergePdfFiles(const QStringList& inputFiles, const QString& outputFile, QString& errorMessage) {
    if (inputFiles.size() < 2) {
        errorMessage = "Mindestens 2 PDF-Dateien sind erforderlich.";
        return false;
    }

    QStringList args;
    args << "--empty" << "--pages";

    for (const QString& file : inputFiles) {
        args << file;
    }

    args << "--" << outputFile;

    QProcess process;
    process.start("qpdf", args);
    process.waitForFinished();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        errorMessage = process.readAllStandardError();
        if (errorMessage.isEmpty()) {
            errorMessage = "qpdf konnte die Dateien nicht mergen.";
        }
        return false;
    }

    return true;
}

bool PdfService::splitPdfFile(const QString& inputFile, const QString& outputFile, const QString& pageRange, QString& errorMessage) {
    if (inputFile.isEmpty()) {
        errorMessage = "Keine Eingabe-PDF ausgewaehlt.";
        return false;
    }

    if (pageRange.isEmpty()) {
        errorMessage = "Kein Seitenbereich angegeben.";
        return false;
    }

    QStringList args;
    args << inputFile << "--pages" << inputFile << pageRange << "--" << outputFile;

    QProcess process;
    process.start("qpdf", args);
    process.waitForFinished();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        errorMessage = process.readAllStandardError();
        if (errorMessage.isEmpty()) {
            errorMessage = "qpdf konnte die PDF nicht splitten.";
        }
        return false;
    }

    return true;
}