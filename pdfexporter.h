#pragma once

#include "salesstore.h"

// Exports the complete filtered/sorted result, not just the visible page.
// Uses a temporary file + atomic commit, preserving an old PDF on failure.
bool exportOrdersPdf(const QString &fileName, const QVector<Order> &orders,
                     const SalesStore &store, const QString &filterSummary,
                     QString *error = nullptr);
