#pragma once

#include <QDate>
#include <QString>
#include <QStringList>
#include <QVector>

// A small in-memory model, independent of the window and its widgets.
// Quantities are per-product; money is stored/calculated in integer cents.
enum class OrderStatus { Draft, Planned, Delivered, Cancelled };
QString statusLabel(OrderStatus status);
QString formatQuantity(double quantity);
QString formatMoney(qint64 cents);

struct Product {
    int id;
    QString name;
    QString shortName;
    QString unit;
    qint64 unitPriceCents;
    double minimumStock;
    bool wholeUnitsOnly;
};

struct Lot {
    QString code;
    int productId;
    double openingQuantity; // Before the demo orders below are deducted.
    QDate expiry;
    bool blocked = false;
};

struct Allocation {
    QString lotCode;
    double quantity;
};

struct Order {
    int id = 0;             // Zero means a new order.
    QDate date;
    QString client;
    int productId = -1;
    double quantity = 0.0;
    OrderStatus status = OrderStatus::Draft;
    QVector<Allocation> allocations;
};

class SalesStore {
public:
    SalesStore();

    const QVector<Product> &products() const { return m_products; }
    const QVector<Lot> &lots() const { return m_lots; }
    const QVector<Order> &orders() const { return m_orders; }
    const Product *product(int id) const;
    const Order *order(int id) const;
    QStringList clients() const;

    double lotRemaining(const QString &code, int excludeOrderId = -1) const;
    double availableStock(int productId, const QDate &onDate = QDate::currentDate(),
                          int excludeOrderId = -1) const;

    // FEFO: unblocked, unexpired lots, earliest expiry first; may split a quantity.
    QVector<Allocation> planLots(int productId, double quantity, const QDate &onDate,
                                int excludeOrderId, QString *error = nullptr) const;

    // Changes are committed only after every validation/allocation check passes.
    bool saveOrder(Order &candidate, QString *error = nullptr);
    bool removeOrder(int id);

    qint64 amountCents(const Order &order) const;
    QString lotText(const Order &order, bool includeQuantities = false) const;

private:
    QVector<Product> m_products;
    QVector<Lot> m_lots;
    QVector<Order> m_orders;
    int m_nextId = 9;
    void seedDemoData();
};
