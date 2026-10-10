#include "pdfexporter.h"

#include <QDateTime>
#include <QFont>
#include <QFontMetrics>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSaveFile>
#include <algorithm>
#include <array>

bool exportOrdersPdf(const QString &fileName, const QVector<Order> &orders,
                     const SalesStore &store, const QString &filterSummary, QString *error)
{
    if (error)
        error->clear();
    QSaveFile output(fileName);
    if (!output.open(QIODevice::WriteOnly)) {
        if (error) *error = output.errorString();
        return false;
    }
    bool success = true;
    {
        // Destroy the writer before committing the file: it must finish the PDF.
        QPdfWriter writer(&output);
        writer.setTitle(QStringLiteral("Liste des commandes"));
        writer.setCreator(QStringLiteral("Gestion des Ventes - Qt"));
        writer.setResolution(120);
        writer.setPageSize(QPageSize(QPageSize::A4));
        writer.setPageOrientation(QPageLayout::Landscape);
        writer.setPageMargins(QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter);
        QPainter painter;
        if (!painter.begin(&writer)) {
            if (error) *error = QStringLiteral("Impossible d'initialiser le document PDF.");
            output.cancelWriting();
            return false;
        }
        painter.setRenderHint(QPainter::Antialiasing);
        const int width = writer.width();
        const int height = writer.height();
        const std::array<double, 8> fractions = {0.04, 0.11, 0.17, 0.16, 0.09, 0.17, 0.13, 0.13};
        std::array<int, 8> columnWidths{};
        int allocated = 0;
        for (int index = 0; index < 7; ++index) {
            columnWidths[index] = static_cast<int>(width * fractions[index]);
            allocated += columnWidths[index];
        }
        columnWidths[7] = width - allocated;
        const QStringList headings = {QStringLiteral("ID"), QStringLiteral("Date"),
                                      QStringLiteral("Client"), QStringLiteral("Article"),
                                      QStringLiteral("Quantité"), QStringLiteral("Lot(s)"),
                                      QStringLiteral("Montant"), QStringLiteral("Statut")};
        QFont bodyFont(QStringLiteral("Segoe UI"), 9);
        int page = 1;
        auto footer = [&] {
            painter.setFont(bodyFont);
            painter.setPen(QColor(QStringLiteral("#6c7b8d")));
            painter.drawText(QRect(0, height - 28, width, 24), Qt::AlignRight | Qt::AlignVCenter,
                             QStringLiteral("Page %1").arg(page));
        };
        auto pageHeader = [&]() -> int {
            painter.fillRect(QRect(0, 0, width, 55), QColor(QStringLiteral("#115fab")));
            QFont titleFont = bodyFont;
            titleFont.setPointSize(17);
            titleFont.setBold(true);
            painter.setFont(titleFont);
            painter.setPen(Qt::white);
            painter.drawText(QRect(14, 0, width - 28, 55), Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("Gestion des Ventes | Liste des commandes"));
            painter.setFont(bodyFont);
            painter.setPen(QColor(QStringLiteral("#54677e")));
            painter.drawText(QRect(0, 68, width, 26), Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("%1 commande(s) | Export du %2")
                                 .arg(orders.size()).arg(QDateTime::currentDateTime().toString(QStringLiteral("dd/MM/yyyy HH:mm"))));
            const QString summary = painter.fontMetrics().elidedText(filterSummary, Qt::ElideRight, width);
            painter.drawText(QRect(0, 98, width, 26), Qt::AlignLeft | Qt::AlignVCenter, summary);
            const int y = 138;
            painter.fillRect(QRect(0, y, width, 36), QColor(QStringLiteral("#e8eff8")));
            QFont headerFont = bodyFont;
            headerFont.setBold(true);
            painter.setFont(headerFont);
            painter.setPen(QColor(QStringLiteral("#1b344f")));
            int x = 0;
            for (int index = 0; index < 8; ++index) {
                painter.drawText(QRect(x + 8, y, columnWidths[index] - 16, 36),
                                 Qt::AlignLeft | Qt::AlignVCenter, headings.at(index));
                x += columnWidths[index];
            }
            painter.setFont(bodyFont);
            return y + 36;
        };
        int y = pageHeader();
        if (orders.isEmpty()) {
            painter.drawText(QRect(0, y + 15, width, 45), Qt::AlignCenter,
                             QStringLiteral("Aucune commande ne correspond aux filtres."));
        }
        for (int index = 0; index < static_cast<int>(orders.size()); ++index) {
            const Order &entry = orders.at(index);
            const Product *article = store.product(entry.productId);
            const QStringList cells = {
                QString::number(entry.id), entry.date.toString(QStringLiteral("dd/MM/yyyy")),
                entry.client, article ? article->name : QStringLiteral("-"),
                formatQuantity(entry.quantity) + QLatin1Char(' ') + (article ? article->unit : QString()),
                store.lotText(entry, true), formatMoney(store.amountCents(entry)), statusLabel(entry.status)
            };
            int rowHeight = 42;
            for (int column = 0; column < 8; ++column) {
                const QRect bounds = painter.fontMetrics().boundingRect(
                    QRect(0, 0, columnWidths[column] - 16, 1000), Qt::TextWordWrap, cells.at(column));
                rowHeight = std::max(rowHeight, bounds.height() + 16);
            }
            if (y + rowHeight > height - 45) {
                footer();
                if (!writer.newPage()) {
                    success = false;
                    if (error) *error = QStringLiteral("Impossible d'ajouter une page au PDF.");
                    break;
                }
                ++page;
                y = pageHeader();
            }
            painter.fillRect(QRect(0, y, width, rowHeight), index % 2 == 0
                                     ? QColor(Qt::white) : QColor(QStringLiteral("#f7f9fc")));
            painter.setPen(QColor(QStringLiteral("#24384e")));
            int x = 0;
            for (int column = 0; column < 8; ++column) {
                painter.drawText(QRect(x + 8, y + 8, columnWidths[column] - 16, rowHeight - 16),
                                 Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, cells.at(column));
                x += columnWidths[column];
            }
            painter.setPen(QColor(QStringLiteral("#e1e6ee")));
            painter.drawLine(0, y + rowHeight, width, y + rowHeight);
            y += rowHeight;
        }
        if (success)
            footer();
        success = painter.end() && success;
    }
    if (!success) {
        output.cancelWriting();
        if (error && error->isEmpty())
            *error = QStringLiteral("Une erreur est survenue pendant la création du PDF.");
        return false;
    }
    if (!output.commit()) {
        if (error) *error = output.errorString();
        return false;
    }
    return true;
}
