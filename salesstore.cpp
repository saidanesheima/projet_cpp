#include "salesstore.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr double Epsilon = 0.000001;

bool usesStock(OrderStatus status)
{
    return status == OrderStatus::Planned || status == OrderStatus::Delivered;
}

QDate eligibilityDate(const QDate &date)
{
    // Backdating a new record must not make an already-expired lot eligible.
    return date.isValid() ? std::max(date, QDate::currentDate()) : QDate::currentDate();
}

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}
}

QString statusLabel(OrderStatus status)
{
    switch (status) {
    case OrderStatus::Draft: return QStringLiteral("Brouillon");
    case OrderStatus::Planned: return QStringLiteral("Planifiée");
    case OrderStatus::Delivered: return QStringLiteral("Livrée");
    case OrderStatus::Cancelled: return QStringLiteral("Annulée");
    }
    return QStringLiteral("Inconnu");
}

QString formatQuantity(double quantity)
{
    QString text = QString::number(quantity, 'f', 2);
    while (text.endsWith(QLatin1Char('0')))
        text.chop(1);
    if (text.endsWith(QLatin1Char('.')))
        text.chop(1);
    return text;
}

QString formatMoney(qint64 cents)
{
    return QString::number(static_cast<double>(cents) / 100.0, 'f', 2)
           + QStringLiteral(" DH");
}

SalesStore::SalesStore()
{
    seedDemoData();
}

void SalesStore::seedDemoData()
{
    m_products = {
        {0, QStringLiteral("Eau purifiée"), QStringLiteral("Eau"),
         QStringLiteral("L"), 250, 200.0, false},
        {1, QStringLiteral("Chlore"), QStringLiteral("Chlore"),
         QStringLiteral("kg"), 1200, 80.0, false},
        {2, QStringLiteral("Flacon 500 ml"), QStringLiteral("Flacon"),
         QStringLiteral("pcs"), 80, 400.0, true}
    };
    const QDate today = QDate::currentDate();
    m_lots = {
        {QStringLiteral("LOT-0413"), 0, 500.0, today.addDays(15), false},
        {QStringLiteral("LOT-0428"), 0, 350.0, today.addDays(100), false},
        {QStringLiteral("LOT-0420"), 1, 140.0, today.addDays(60), false},
        {QStringLiteral("LOT-0430"), 2, 450.0, today.addDays(240), false},
        {QStringLiteral("LOT-0422"), 1, 35.0, today.addDays(50), true},
        {QStringLiteral("LOT-0401"), 0, 20.0, today.addDays(-5), false}
    };
    // Same five visible orders as the reference, plus three water deliveries.
    // Planned orders reserve stock; deliveries consume stock. Both are deducted
    // once from openingQuantity. Drafts/cancellations do not deduct stock.
    m_orders = {
        {1, QDate(2026, 8, 15), QStringLiteral("Pharma Atlas"), 0, 200.0,
         OrderStatus::Delivered, {{QStringLiteral("LOT-0413"), 200.0}}},
        {2, QDate(2026, 9, 18), QStringLiteral("Clinique Nour"), 0, 180.0,
         OrderStatus::Delivered, {{QStringLiteral("LOT-0413"), 180.0}}},
        {3, QDate(2026, 10, 1), QStringLiteral("Hôtel Riad"), 0, 20.0,
         OrderStatus::Delivered, {{QStringLiteral("LOT-0413"), 20.0}}},
        {4, QDate(2026, 9, 28), QStringLiteral("Clinique Nour"), 1, 30.0,
         OrderStatus::Delivered, {{QStringLiteral("LOT-0420"), 30.0}}},
        {5, QDate(2026, 10, 1), QStringLiteral("Hôtel Riad"), 1, 20.0,
         OrderStatus::Delivered, {{QStringLiteral("LOT-0420"), 20.0}}},
        {6, QDate(2026, 10, 2), QStringLiteral("Pharma Atlas"), 2, 100.0,
         OrderStatus::Planned, {{QStringLiteral("LOT-0430"), 100.0}}},
        {7, QDate(2026, 10, 4), QStringLiteral("Hôtel Riad"), 0, 60.0,
         OrderStatus::Draft, {}},
        {8, QDate(2026, 10, 4), QStringLiteral("Clinique Nour"), 2, 30.0,
         OrderStatus::Cancelled, {}}
    };
}

const Product *SalesStore::product(int id) const
{
    for (const Product &entry : m_products)
        if (entry.id == id)
            return &entry;
    return nullptr;
}

const Order *SalesStore::order(int id) const
{
    for (const Order &entry : m_orders)
        if (entry.id == id)
            return &entry;
    return nullptr;
}

QStringList SalesStore::clients() const
{
    QStringList result;
    for (const Order &entry : m_orders)
        if (!result.contains(entry.client, Qt::CaseInsensitive))
            result.append(entry.client);
    result.sort(Qt::CaseInsensitive);
    return result;
}

double SalesStore::lotRemaining(const QString &code, int excludeOrderId) const
{
    const auto found = std::find_if(m_lots.cbegin(), m_lots.cend(),
                                   [&code](const Lot &lot) { return lot.code == code; });
    if (found == m_lots.cend())
        return 0.0;
    double remaining = found->openingQuantity;
    for (const Order &entry : m_orders) {
        if (entry.id == excludeOrderId || !usesStock(entry.status))
            continue;
        for (const Allocation &allocation : entry.allocations)
            if (allocation.lotCode == code)
                remaining -= allocation.quantity;
    }
    return std::max(0.0, remaining);
}

double SalesStore::availableStock(int productId, const QDate &onDate,
                                  int excludeOrderId) const
{
    double total = 0.0;
    const QDate validOn = eligibilityDate(onDate);
    for (const Lot &lot : m_lots)
        if (lot.productId == productId && !lot.blocked && lot.expiry >= validOn)
            total += lotRemaining(lot.code, excludeOrderId);
    return total;
}

QVector<Allocation> SalesStore::planLots(int productId, double quantity,
                                       const QDate &onDate, int excludeOrderId,
                                       QString *error) const
{
    setError(error, QString());
    const Product *article = product(productId);
    if (!article || !std::isfinite(quantity) || quantity <= 0.0) {
        setError(error, QStringLiteral("Article ou quantité invalide."));
        return {};
    }
    const double stock = availableStock(productId, onDate, excludeOrderId);
    if (quantity > stock + Epsilon) {
        setError(error, QStringLiteral("Stock insuffisant : %1 %2 disponible(s).")
                            .arg(formatQuantity(stock), article->unit));
        return {};
    }
    QVector<const Lot *> eligible;
    const QDate validOn = eligibilityDate(onDate);
    for (const Lot &lot : m_lots)
        if (lot.productId == productId && !lot.blocked && lot.expiry >= validOn
            && lotRemaining(lot.code, excludeOrderId) > Epsilon)
            eligible.append(&lot);
    std::sort(eligible.begin(), eligible.end(), [](const Lot *a, const Lot *b) {
        return a->expiry == b->expiry ? a->code < b->code : a->expiry < b->expiry;
    });

    QVector<Allocation> result;
    double needed = quantity;
    for (const Lot *lot : eligible) {
        const double used = std::min(needed, lotRemaining(lot->code, excludeOrderId));
        if (used > Epsilon)
            result.append({lot->code, used});
        needed -= used;
        if (needed <= Epsilon)
            break;
    }
    if (needed > Epsilon) {
        setError(error, QStringLiteral("Aucun lot disponible pour cette quantité."));
        return {};
    }
    return result;
}

bool SalesStore::saveOrder(Order &candidate, QString *error)
{
    setError(error, QString());
    Order validated = candidate;
    validated.client = validated.client.simplified();
    if (validated.id != 0 && !order(validated.id)) {
        setError(error, QStringLiteral("Cette commande n'existe plus."));
        return false;
    }
    if (!validated.date.isValid()) {
        setError(error, QStringLiteral("Choisissez une date valide."));
        return false;
    }
    if (validated.client.isEmpty() || validated.client.size() > 80) {
        setError(error, QStringLiteral("Le client doit contenir entre 1 et 80 caractères."));
        return false;
    }
    // Avoid duplicate client filter entries that differ only by case.
    for (const QString &known : clients())
        if (known.compare(validated.client, Qt::CaseInsensitive) == 0) {
            validated.client = known;
            break;
        }
    const Product *article = product(validated.productId);
    if (!article || !std::isfinite(validated.quantity) || validated.quantity <= 0.0
        || validated.quantity > 1000000.0) {
        setError(error, QStringLiteral("Choisissez un article et une quantité entre 0 et 1 000 000 (hors zéro)."));
        return false;
    }
    const double scale = article->wholeUnitsOnly ? 1.0 : 100.0;
    if (std::abs(validated.quantity * scale - std::round(validated.quantity * scale)) > Epsilon) {
        setError(error, article->wholeUnitsOnly
                           ? QStringLiteral("Les flacons doivent être comptés en nombres entiers.")
                           : QStringLiteral("La quantité accepte au maximum deux décimales."));
        return false;
    }
    switch (validated.status) {
    case OrderStatus::Draft:
    case OrderStatus::Cancelled:
        validated.allocations.clear();
        break;
    case OrderStatus::Planned:
    case OrderStatus::Delivered: {
        QString allocationError;
        validated.allocations = planLots(validated.productId, validated.quantity,
                                         validated.date, validated.id, &allocationError);
        if (!allocationError.isEmpty()) {
            setError(error, allocationError);
            return false;
        }
        break;
    }
    default:
        setError(error, QStringLiteral("Statut invalide."));
        return false;
    }
    if (validated.id == 0) {
        validated.id = m_nextId++;
        m_orders.append(validated);
    } else {
        for (Order &entry : m_orders)
            if (entry.id == validated.id) {
                entry = validated;
                break;
            }
    }
    candidate = validated;
    return true;
}

bool SalesStore::removeOrder(int id)
{
    for (int index = 0; index < static_cast<int>(m_orders.size()); ++index)
        if (m_orders.at(index).id == id) {
            m_orders.removeAt(index);
            return true;
        }
    return false;
}

qint64 SalesStore::amountCents(const Order &entry) const
{
    const Product *article = product(entry.productId);
    return article ? static_cast<qint64>(std::llround(entry.quantity * article->unitPriceCents)) : 0;
}

QString SalesStore::lotText(const Order &entry, bool includeQuantities) const
{
    QStringList result;
    const Product *article = product(entry.productId);
    for (const Allocation &allocation : entry.allocations) {
        QString text = allocation.lotCode;
        if (includeQuantities && article)
            text += QStringLiteral(" (%1 %2)").arg(formatQuantity(allocation.quantity), article->unit);
        result.append(text);
    }
    return result.isEmpty() ? QStringLiteral("-")
                            : result.join(includeQuantities ? QStringLiteral("\n") : QStringLiteral(", "));
}
