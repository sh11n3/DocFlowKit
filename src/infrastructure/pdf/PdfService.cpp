#include "PdfService.h"

#include <QProcess>
#include <QDebug>
#include <QFileInfo>

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
    process.start("py", QStringList() << "-m" << "ocrmypdf" << args);
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

bool PdfService::runOcrOnPdf(const QString& inputFile, const QString& outputFile, QString& errorMessage)
{
    if (inputFile.isEmpty()) {
        errorMessage = "Keine Eingabe-PDF ausgewaehlt.";
        return false;
    }

    if (outputFile.isEmpty()) {
        errorMessage = "Kein Ausgabe-Pfad angegeben.";
        return false;
    }

    QStringList args;
    args << "--force-ocr"
         << "--output-type" << "pdf"
         << "-l" << "deu+eng"
         << inputFile
         << outputFile;

    QProcess process;
    process.start("py", QStringList() << "-m" << "ocrmypdf" << args);
    process.waitForFinished(-1);

    QString stdOut = process.readAllStandardOutput();
    QString stdErr = process.readAllStandardError();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        errorMessage = stdErr;
        if (errorMessage.isEmpty()) {
            errorMessage = stdOut;
        }
        if (errorMessage.isEmpty()) {
            errorMessage = "OCRmyPDF konnte die PDF nicht verarbeiten.";
        }
        return false;
    }

    if (!QFileInfo::exists(outputFile)) {
        errorMessage = "OCR lief ohne Fehler, aber die Ausgabedatei wurde nicht gefunden.";
        return false;
    }

    return true;
}