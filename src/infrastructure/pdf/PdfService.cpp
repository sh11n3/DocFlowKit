#include "PdfService.h"

#include <QProcess>

bool PdfService::mergePdfPages(const QStringList& pageSpecs, const QString& outputFile, QString& errorMessage) {
    if (pageSpecs.isEmpty()) {
        errorMessage = "Keine Seiten zum Mergen vorhanden.";
        return false;
    }

    QStringList args;
    args << "--empty" << "--pages";

    for (const QString& spec : pageSpecs) {
        args << spec;
    }

    args << "--" << outputFile;

    QProcess process;
    process.start("qpdf", args);
    process.waitForFinished();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        errorMessage = process.readAllStandardError();
        if (errorMessage.isEmpty()) {
            errorMessage = "qpdf konnte die Seiten nicht mergen.";
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