#include "mainwindow.h"
#include "barchartwidget.h"
#include "pdfexporter.h"

#include <QAbstractItemView>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPair>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QStringListModel>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <array>

namespace {
const QColor Blue(QStringLiteral("#2282d4"));
const QColor Green(QStringLiteral("#29a373"));
const QColor Amber(QStringLiteral("#e0a131"));
const QColor Red(QStringLiteral("#c82b27"));
const std::array<OrderStatus, 4> Statuses = {
    OrderStatus::Draft, OrderStatus::Planned, OrderStatus::Delivered, OrderStatus::Cancelled
};

enum class IconKind { Cube, Search, Edit, Delete, Previous, Next };

// Draw crisp, self-contained icons. No image files or Qt SVG plugin needed.
QIcon makeIcon(IconKind kind, const QColor &color, int size = 20)
{
    QPixmap image(size * 2, size * 2);
    image.setDevicePixelRatio(2.0);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(static_cast<double>(size) / 22.0, static_cast<double>(size) / 22.0);
    painter.setPen(QPen(color, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    switch (kind) {
    case IconKind::Cube: {
        QPainterPath outline;
        outline.moveTo(11, 2); outline.lineTo(20, 6.5); outline.lineTo(20, 16);
        outline.lineTo(11, 20.5); outline.lineTo(2, 16); outline.lineTo(2, 6.5);
        outline.closeSubpath();
        painter.drawPath(outline);
        painter.drawLine(QPointF(2, 6.5), QPointF(11, 11));
        painter.drawLine(QPointF(20, 6.5), QPointF(11, 11));
        painter.drawLine(QPointF(11, 11), QPointF(11, 20.5));
        break;
    }
    case IconKind::Search:
        painter.drawEllipse(QRectF(4, 4, 10, 10));
        painter.drawLine(QPointF(12.5, 12.5), QPointF(18, 18));
        break;
    case IconKind::Edit: {
        QPainterPath pencil;
        pencil.moveTo(4, 17.8); pencil.lineTo(5.4, 12.5); pencil.lineTo(15.3, 2.7);
        pencil.lineTo(19, 6.4); pencil.lineTo(9, 16.3); pencil.closeSubpath();
        painter.drawPath(pencil);
        painter.drawLine(QPointF(13.4, 4.6), QPointF(17.1, 8.3));
        painter.drawLine(QPointF(5.4, 12.5), QPointF(9, 16.3));
        break;
    }
    case IconKind::Delete:
        painter.drawRoundedRect(QRectF(6, 7, 10, 12), 1, 1);
        painter.drawLine(QPointF(4, 5), QPointF(18, 5));
        painter.drawLine(QPointF(9, 2.8), QPointF(13, 2.8));
        painter.drawLine(QPointF(9, 10), QPointF(9, 16));
        painter.drawLine(QPointF(13, 10), QPointF(13, 16));
        break;
    case IconKind::Previous:
        painter.drawLine(QPointF(13, 7), QPointF(9, 11));
        painter.drawLine(QPointF(9, 11), QPointF(13, 15));
        break;
    case IconKind::Next:
        painter.drawLine(QPointF(9, 7), QPointF(13, 11));
        painter.drawLine(QPointF(13, 11), QPointF(9, 15));
        break;
    }
    painter.end();
    return QIcon(image);
}

QFrame *makePanel(QWidget *parent)
{
    auto *panel = new QFrame(parent);
    panel->setProperty("panel", true);
    return panel;
}

QLabel *makeHeading(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("heading", true);
    return label;
}

QLineEdit *makeReadOnlyField(QWidget *parent, const QString &name)
{
    auto *field = new QLineEdit(parent);
    field->setObjectName(name);
    field->setReadOnly(true);
    field->setFocusPolicy(Qt::NoFocus);
    field->setFixedHeight(36);
    return field;
}

// Percentage widths keep the table close to the reference at different sizes.
class OrdersTable final : public QTableWidget {
public:
    explicit OrdersTable(QWidget *parent = nullptr) : QTableWidget(parent) {}

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QTableWidget::resizeEvent(event);
        if (columnCount() != 9)
            return;
        const std::array<double, 9> widths = {0.04, 0.12, 0.14, 0.14, 0.10, 0.11, 0.12, 0.14, 0.09};
        const int total = viewport()->width();
        int used = 0;
        for (int column = 0; column < 8; ++column) {
            const int width = static_cast<int>(total * widths[column]);
            setColumnWidth(column, width);
            used += width;
        }
        setColumnWidth(8, std::max(1, total - used));
        if (auto *hint = viewport()->findChild<QLabel *>(QStringLiteral("EmptyState")))
            hint->setGeometry(viewport()->rect());
    }
};

class StatusDelegate final : public QStyledItemDelegate {
public:
    explicit StatusDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem background(option);
        initStyleOption(&background, index);
        const QString text = background.text;
        background.text.clear();
        const QWidget *widget = option.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &background, painter, widget);

        QColor fill(QStringLiteral("#e8eff8"));
        QColor ink(QStringLiteral("#63768e"));
        const auto status = static_cast<OrderStatus>(index.data(Qt::UserRole).toInt());
        switch (status) {
        case OrderStatus::Planned:
            fill = QColor(QStringLiteral("#fff0d8")); ink = QColor(QStringLiteral("#806020")); break;
        case OrderStatus::Delivered:
            fill = QColor(QStringLiteral("#d9f0e0")); ink = QColor(QStringLiteral("#268246")); break;
        case OrderStatus::Cancelled:
            fill = QColor(QStringLiteral("#fde1df")); ink = QColor(QStringLiteral("#be302c")); break;
        case OrderStatus::Draft: break;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QFont badgeFont = option.font;
        badgeFont.setBold(true);
        painter->setFont(badgeFont);
        const int pillWidth = std::min(option.rect.width() - 10,
                                       painter->fontMetrics().horizontalAdvance(text) + 16);
        const QRect pill(option.rect.left() + 5, option.rect.center().y() - 12,
                         std::max(1, pillWidth), 25);
        painter->setPen(Qt::NoPen);
        painter->setBrush(fill);
        painter->drawRoundedRect(pill, 5, 5);
        painter->setPen(ink);
        painter->drawText(pill.adjusted(6, 0, -6, 0), Qt::AlignCenter,
                          painter->fontMetrics().elidedText(text, Qt::ElideRight, pill.width() - 12));
        painter->restore();
    }
};
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Gestion des Ventes"));
    setWindowIcon(makeIcon(IconKind::Cube, QColor(QStringLiteral("#115fab")), 32));
    resize(1220, 850);
    setMinimumSize(1100, 820);
    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("AppRoot"));
    setCentralWidget(root);
    auto *outer = new QVBoxLayout(root);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addWidget(createHeader());

    auto *body = new QHBoxLayout;
    body->setContentsMargins(12, 14, 12, 14);
    body->setSpacing(14);
    body->addWidget(createFormPanel());
    auto *right = new QVBoxLayout;
    right->setSpacing(14);
    right->addWidget(createOrdersPanel());
    right->addWidget(createStatisticsPanel(), 1);
    body->addLayout(right, 1);
    outer->addLayout(body, 1);

    connectSignals();
    updateClientChoices();
    clearForm();
    refreshTable();
    updateDiagnostics();
    updateCharts();
}

QFrame *MainWindow::createHeader()
{
    auto *header = new QFrame(this);
    header->setObjectName(QStringLiteral("TopBar"));
    header->setFixedHeight(48);
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(14, 0, 14, 0);
    layout->setSpacing(12);
    auto *logo = new QLabel(header);
    logo->setPixmap(makeIcon(IconKind::Cube, Qt::white, 23).pixmap(23, 23));
    logo->setFixedWidth(23);
    auto *title = new QLabel(QStringLiteral("Gestion des Ventes"), header);
    title->setObjectName(QStringLiteral("BrandTitle"));
    layout->addWidget(logo);
    layout->addWidget(title);
    layout->addStretch();
    auto *account = new QToolButton(header);
    account->setObjectName(QStringLiteral("AccountButton"));
    account->setText(QStringLiteral("Responsable ventes"));
    account->setPopupMode(QToolButton::InstantPopup);
    account->setCursor(Qt::PointingHandCursor);
    auto *menu = new QMenu(account);
    auto *about = menu->addAction(QStringLiteral("À propos de cette démo"));
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::information(this, QStringLiteral("Gestion des Ventes"),
            QStringLiteral("Interface C++ / Qt Widgets.\n\n"
                           "Les données sont conservées en mémoire uniquement.\n"
                           "Elles sont réinitialisées à chaque lancement.\n"
                           "Le menu Responsable ventes n'est pas une authentification."));
    });
    account->setMenu(menu);
    layout->addWidget(account);
    return header;
}

QFrame *MainWindow::createFormPanel()
{
    auto *panel = makePanel(this);
    panel->setObjectName(QStringLiteral("FormPanel"));
    panel->setFixedWidth(286);
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    m_formTitle = makeHeading(QStringLiteral("Ajouter une nouvelle\ncommande"), panel);
    m_formTitle->setObjectName(QStringLiteral("FormTitle"));
    m_formTitle->setWordWrap(true);
    layout->addWidget(m_formTitle);

    auto *form = new QGridLayout;
    form->setHorizontalSpacing(9);
    form->setVerticalSpacing(8);
    form->setColumnMinimumWidth(0, 80);
    form->setColumnStretch(1, 1);
    auto addRow = [&](int row, const QString &caption, QWidget *field) {
        auto *label = new QLabel(caption, panel);
        label->setBuddy(field);
        form->addWidget(label, row, 0);
        form->addWidget(field, row, 1);
    };

    m_dateEdit = new QDateEdit(QDate::currentDate(), panel);
    m_dateEdit->setObjectName(QStringLiteral("dateEdit"));
    m_dateEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setFixedHeight(36);
    addRow(0, QStringLiteral("Date *"), m_dateEdit);

    m_clientEdit = new QLineEdit(panel);
    m_clientEdit->setObjectName(QStringLiteral("clientEdit"));
    m_clientEdit->setPlaceholderText(QStringLiteral("ex. Pharma Atlas"));
    m_clientEdit->setMaxLength(80);
    m_clientEdit->setFixedHeight(36);
    m_clientsModel = new QStringListModel(this);
    auto *completer = new QCompleter(m_clientsModel, this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    m_clientEdit->setCompleter(completer);
    addRow(1, QStringLiteral("Client *"), m_clientEdit);

    m_articleCombo = new QComboBox(panel);
    m_articleCombo->setObjectName(QStringLiteral("articleCombo"));
    m_articleCombo->setFixedHeight(36);
    for (const Product &article : m_store.products())
        m_articleCombo->addItem(article.name, article.id);
    addRow(2, QStringLiteral("Article *"), m_articleCombo);

    m_quantityEdit = new QDoubleSpinBox(panel);
    m_quantityEdit->setObjectName(QStringLiteral("quantityEdit"));
    m_quantityEdit->setLocale(QLocale::c());
    m_quantityEdit->setRange(0.0, 1000000.0);
    m_quantityEdit->setDecimals(2);
    m_quantityEdit->setSingleStep(10.0);
    m_quantityEdit->setSpecialValueText(QStringLiteral("ex. 50"));
    m_quantityEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_quantityEdit->setFixedHeight(36);
    m_quantityEdit->setKeyboardTracking(false);
    addRow(3, QStringLiteral("Quantité *"), m_quantityEdit);

    m_statusCombo = new QComboBox(panel);
    m_statusCombo->setObjectName(QStringLiteral("statusCombo"));
    m_statusCombo->setFixedHeight(36);
    for (OrderStatus status : Statuses)
        m_statusCombo->addItem(statusLabel(status), static_cast<int>(status));
    m_statusCombo->setToolTip(QStringLiteral("Planifiée : réserve le stock. Livrée : consomme le stock.\n"
                                           "Brouillon et Annulée : aucun mouvement de stock."));
    addRow(4, QStringLiteral("Statut"), m_statusCombo);
    m_stockEdit = makeReadOnlyField(panel, QStringLiteral("stockEdit"));
    addRow(5, QStringLiteral("Stock dispo"), m_stockEdit);
    m_lotEdit = makeReadOnlyField(panel, QStringLiteral("lotEdit"));
    addRow(6, QStringLiteral("Lot (FEFO)"), m_lotEdit);
    m_amountEdit = makeReadOnlyField(panel, QStringLiteral("amountEdit"));
    addRow(7, QStringLiteral("Montant"), m_amountEdit);
    layout->addLayout(form);

    auto *control = new QHBoxLayout;
    control->setSpacing(9);
    auto *controlLabel = new QLabel(QStringLiteral("Contrôle"), panel);
    controlLabel->setFixedWidth(80);
    control->addWidget(controlLabel);
    m_controlBadge = new QLabel(panel);
    m_controlBadge->setObjectName(QStringLiteral("ControlBadge"));
    m_controlBadge->setProperty("state", QStringLiteral("neutral"));
    m_controlBadge->setMinimumWidth(0);
    m_controlBadge->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    control->addWidget(m_controlBadge, 1);
    layout->addLayout(control);

    auto *buttons = new QHBoxLayout;
    buttons->setSpacing(9);
    m_saveButton = new QPushButton(QStringLiteral("+ Ajouter"), panel);
    m_saveButton->setObjectName(QStringLiteral("saveButton"));
    m_saveButton->setProperty("primary", true);
    m_saveButton->setFixedHeight(36);
    m_saveButton->setCursor(Qt::PointingHandCursor);
    m_clearButton = new QPushButton(QStringLiteral("Vider"), panel);
    m_clearButton->setObjectName(QStringLiteral("clearButton"));
    m_clearButton->setFixedHeight(36);
    m_clearButton->setCursor(Qt::PointingHandCursor);
    buttons->addWidget(m_saveButton, 1);
    buttons->addWidget(m_clearButton, 1);
    layout->addLayout(buttons);

    m_feedbackLabel = new QLabel(panel);
    m_feedbackLabel->setProperty("small", true);
    m_feedbackLabel->setProperty("muted", true);
    m_feedbackLabel->setMinimumHeight(15);
    m_feedbackLabel->setWordWrap(true);
    layout->addWidget(m_feedbackLabel);
    m_diagnostics = new QLabel(panel);
    m_diagnostics->setObjectName(QStringLiteral("Diagnostics"));
    m_diagnostics->setTextFormat(Qt::RichText);
    m_diagnostics->setWordWrap(true);
    m_diagnostics->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_diagnostics->setMinimumHeight(160);
    layout->addWidget(m_diagnostics);
    layout->addStretch(1);
    return panel;
}

QFrame *MainWindow::createOrdersPanel()
{
    auto *panel = makePanel(this);
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 16, 16, 14);
    layout->setSpacing(12);
    auto *title = makeHeading(QStringLiteral("Liste des commandes"), panel);
    title->setObjectName(QStringLiteral("OrdersTitle"));
    layout->addWidget(title);

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);
    m_searchEdit = new QLineEdit(panel);
    m_searchEdit->setObjectName(QStringLiteral("searchEdit"));
    m_searchEdit->setPlaceholderText(QStringLiteral("Recherche par article, client, n° ou lot"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->addAction(makeIcon(IconKind::Search, QColor(QStringLiteral("#263c55")), 16),
                            QLineEdit::LeadingPosition);
    m_searchEdit->setFixedHeight(36);
    m_searchEdit->setMinimumWidth(175);
    toolbar->addWidget(m_searchEdit, 1);

    auto *sortButton = new QPushButton(QStringLiteral("Trier"), panel);
    sortButton->setFixedHeight(36);
    sortButton->setMinimumWidth(66);
    auto *sortMenu = new QMenu(sortButton);
    auto *sortGroup = new QActionGroup(sortMenu);
    sortGroup->setExclusive(true);
    const QVector<QPair<QString, SortMode>> options = {
        {QStringLiteral("Date : plus récente"), SortMode::DateNewest},
        {QStringLiteral("Date : plus ancienne"), SortMode::DateOldest}
    };
    for (const auto &option : options) {
        auto *action = sortMenu->addAction(option.first);
        action->setCheckable(true);
        action->setChecked(option.second == m_sortMode);
        sortGroup->addAction(action);
        const SortMode mode = option.second;
        connect(action, &QAction::triggered, this, [this, mode] {
            m_sortMode = mode;
            m_page = 1;
            refreshTable();
        });
    }
    sortButton->setMenu(sortMenu);
    toolbar->addWidget(sortButton);
    m_exportButton = new QPushButton(QStringLiteral("Exporter PDF"), panel);
    m_exportButton->setObjectName(QStringLiteral("exportButton"));
    m_exportButton->setFixedHeight(36);
    m_exportButton->setToolTip(QStringLiteral("Exporter toutes les commandes filtrées, sur toutes les pages."));
    toolbar->addWidget(m_exportButton);
    m_statusFilter = new QComboBox(panel);
    m_statusFilter->setObjectName(QStringLiteral("statusFilter"));
    m_statusFilter->setFixedSize(119, 36);
    m_statusFilter->addItem(QStringLiteral("Tous statuts"), -1);
    for (OrderStatus status : Statuses)
        m_statusFilter->addItem(statusLabel(status), static_cast<int>(status));
    toolbar->addWidget(m_statusFilter);
    m_clientFilter = new QComboBox(panel);
    m_clientFilter->setObjectName(QStringLiteral("clientFilter"));
    m_clientFilter->setFixedSize(125, 36);
    toolbar->addWidget(m_clientFilter);
    layout->addLayout(toolbar);

    auto *tableFrame = new QFrame(panel);
    tableFrame->setObjectName(QStringLiteral("TableFrame"));
    auto *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(1, 1, 1, 1);
    m_table = new OrdersTable(tableFrame);
    m_table->setObjectName(QStringLiteral("ordersTable"));
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({QStringLiteral("ID"), QStringLiteral("Date"),
        QStringLiteral("Client"), QStringLiteral("Article"), QStringLiteral("Quantité"),
        QStringLiteral("Lot"), QStringLiteral("Montant"), QStringLiteral("Statut"), QStringLiteral("Actions")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(47);
    m_table->horizontalHeader()->setFixedHeight(38);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    m_table->horizontalHeader()->setMinimumSectionSize(20);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setFixedHeight(38 + PageSize * 47);
    m_table->setItemDelegateForColumn(7, new StatusDelegate(m_table));
    m_emptyLabel = new QLabel(QStringLiteral("Aucune commande ne correspond aux filtres."), m_table->viewport());
    m_emptyLabel->setObjectName(QStringLiteral("EmptyState"));
    m_emptyLabel->setProperty("muted", true);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_emptyLabel->hide();
    tableLayout->addWidget(m_table);
    layout->addWidget(tableFrame);

    auto *pagination = new QHBoxLayout;
    pagination->addStretch();
    m_pageLabel = new QLabel(panel);
    m_pageLabel->setProperty("muted", true);
    pagination->addWidget(m_pageLabel);
    pagination->addSpacing(3);
    m_pageButtonsLayout = new QHBoxLayout;
    m_pageButtonsLayout->setSpacing(6);
    pagination->addLayout(m_pageButtonsLayout);
    layout->addLayout(pagination);
    return panel;
}

QFrame *MainWindow::createStatisticsPanel()
{
    auto *panel = makePanel(this);
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 15, 16, 16);
    layout->setSpacing(13);
    layout->addWidget(makeHeading(QStringLiteral("Statistiques des ventes"), panel));
    auto *cards = new QHBoxLayout;
    cards->setSpacing(12);
    auto addChart = [&](const QString &title, BarChartWidget *&chart, QLabel *&subtitle,
                        const QString &note) {
        auto *card = new QFrame(panel);
        card->setProperty("chartCard", true);
        card->setMinimumWidth(0);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(13, 12, 13, 12);
        cardLayout->setSpacing(3);
        auto *heading = new QLabel(title, card);
        heading->setProperty("chartTitle", true);
        heading->setMinimumWidth(0);
        heading->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        cardLayout->addWidget(heading);
        subtitle = new QLabel(card);
        subtitle->setProperty("small", true);
        subtitle->setProperty("muted", true);
        subtitle->setMinimumWidth(0);
        subtitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        cardLayout->addWidget(subtitle);
        chart = new BarChartWidget(card);
        cardLayout->addWidget(chart, 1);
        auto *legend = new QLabel(note, card);
        legend->setProperty("small", true);
        legend->setProperty("muted", true);
        legend->setWordWrap(true);
        legend->setMinimumHeight(29);
        cardLayout->addWidget(legend);
        cards->addWidget(card, 1);
    };
    addChart(QStringLiteral("Volume livré par client"), m_clientChart, m_clientChartSubtitle, QString());
    addChart(QStringLiteral("Volume livré par mois"), m_monthChart, m_monthChartSubtitle, QString());
    QLabel *stockSubtitle = nullptr;
    addChart(QStringLiteral("Niveau de stock vs seuil"), m_stockChart, stockSubtitle,
             QStringLiteral("Trait orange : seuil minimum.\nRouge : stock sous le seuil."));
    stockSubtitle->setText(QStringLiteral("Stock disponible par article"));
    stockSubtitle->setToolTip(QStringLiteral("Chaque article conserve son unité : L, kg ou pcs.\n"
                                            "Ces unités ne sont pas additionnées."));
    layout->addLayout(cards, 1);
    return panel;
}

void MainWindow::connectSignals()
{
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveOrder);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::clearForm);
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::exportPdf);
    connect(m_clientEdit, &QLineEdit::textChanged, this, &MainWindow::updateDerivedFields);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &MainWindow::updateDerivedFields);
    connect(m_quantityEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::updateDerivedFields);
    connect(m_statusCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::updateDerivedFields);
    connect(m_articleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (const Product *article = m_store.product(m_articleCombo->currentData().toInt())) {
            const QSignalBlocker block(m_quantityEdit);
            m_quantityEdit->setDecimals(article->wholeUnitsOnly ? 0 : 2);
            m_quantityEdit->setSuffix(QLatin1Char(' ') + article->unit);
        }
        updateDerivedFields();
        updateCharts();
    });
    auto refilter = [this] { m_page = 1; refreshTable(); };
    connect(m_searchEdit, &QLineEdit::textChanged, this, refilter);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refilter);
    connect(m_clientFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, refilter);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (QTableWidgetItem *item = m_table->item(row, 0))
            editOrder(item->data(Qt::UserRole).toInt());
    });
    QWidget::setTabOrder(m_dateEdit, m_clientEdit);
    QWidget::setTabOrder(m_clientEdit, m_articleCombo);
    QWidget::setTabOrder(m_articleCombo, m_quantityEdit);
    QWidget::setTabOrder(m_quantityEdit, m_statusCombo);
    QWidget::setTabOrder(m_statusCombo, m_saveButton);
    QWidget::setTabOrder(m_saveButton, m_clearButton);
    QWidget::setTabOrder(m_clearButton, m_searchEdit);
}

Order MainWindow::orderFromForm() const
{
    Order result;
    result.id = m_editingId;
    result.date = m_dateEdit->date();
    result.client = m_clientEdit->text().simplified();
    result.productId = m_articleCombo->currentData().toInt();
    result.quantity = m_quantityEdit->value();
    result.status = static_cast<OrderStatus>(m_statusCombo->currentData().toInt());
    return result;
}

void MainWindow::setControlState(const QString &text, const QString &state, const QString &tooltip)
{
    m_controlBadge->setText(text);
    m_controlBadge->setToolTip(tooltip.isEmpty() ? text : tooltip);
    if (m_controlBadge->property("state").toString() != state) {
        m_controlBadge->setProperty("state", state);
        m_controlBadge->style()->unpolish(m_controlBadge);
        m_controlBadge->style()->polish(m_controlBadge);
    }
}

void MainWindow::updateDerivedFields()
{
    Order preview = orderFromForm();
    const Product *article = m_store.product(preview.productId);
    if (!article)
        return;
    const double stock = m_store.availableStock(article->id, preview.date, m_editingId);
    m_stockEdit->setText(formatQuantity(stock) + QLatin1Char(' ') + article->unit);
    m_stockEdit->setToolTip(QStringLiteral("Stock libre, non bloqué et non périmé pour la date choisie.\n"
                                         "En modification, le stock de la commande courante est remis à disposition."));
    m_amountEdit->setText(preview.quantity > 0.0 ? formatMoney(m_store.amountCents(preview)) : QStringLiteral("-"));
    m_amountEdit->setToolTip(QStringLiteral("Prix unitaire : %1 / %2")
                               .arg(formatMoney(article->unitPriceCents), article->unit));
    m_lotEdit->setText(QStringLiteral("-"));
    m_lotEdit->setToolTip(QStringLiteral("Les brouillons et commandes annulées ne réservent aucun lot."));
    QString allocationError;
    if (preview.quantity > 0.0 && (preview.status == OrderStatus::Planned || preview.status == OrderStatus::Delivered)) {
        preview.allocations = m_store.planLots(preview.productId, preview.quantity, preview.date,
                                               m_editingId, &allocationError);
        m_lotEdit->setText(m_store.lotText(preview));
        m_lotEdit->setCursorPosition(0);
        m_lotEdit->setToolTip(allocationError.isEmpty() ? m_store.lotText(preview, true) : allocationError);
    }
    if (preview.client.isEmpty() || preview.quantity <= 0.0) {
        setControlState(QStringLiteral("À compléter"), QStringLiteral("neutral"),
                        QStringLiteral("Renseignez le client et une quantité positive."));
    } else if (!allocationError.isEmpty()) {
        setControlState(QStringLiteral("Stock insuffisant"), QStringLiteral("danger"), allocationError);
    } else if (preview.status == OrderStatus::Draft && preview.quantity > stock) {
        setControlState(QStringLiteral("Stock insuffisant"), QStringLiteral("warning"),
                        QStringLiteral("Le brouillon peut être enregistré, mais pas planifié ni livré avec ce stock."));
    } else {
        setControlState(QStringLiteral("Conforme (calculé)"), QStringLiteral("success"));
    }
}

void MainWindow::updateClientChoices()
{
    const QString previous = m_clientFilter->currentData().toString();
    const QStringList clients = m_store.clients();
    m_clientsModel->setStringList(clients);
    const QSignalBlocker block(m_clientFilter);
    m_clientFilter->clear();
    m_clientFilter->addItem(QStringLiteral("Tous clients"), QString());
    for (const QString &client : clients)
        m_clientFilter->addItem(client, client);
    const int index = m_clientFilter->findData(previous);
    m_clientFilter->setCurrentIndex(std::max(0, index));
}

void MainWindow::refreshTable()
{
    const QString query = m_searchEdit->text().trimmed();
    const int status = m_statusFilter->currentData().toInt();
    const QString client = m_clientFilter->currentData().toString();
    m_filteredIds.clear();
    for (const Order &entry : m_store.orders()) {
        if (status != -1 && static_cast<int>(entry.status) != status)
            continue;
        if (!client.isEmpty() && entry.client != client)
            continue;
        const Product *article = m_store.product(entry.productId);
        const QString searchable = QString::number(entry.id) + QLatin1Char(' ')
            + entry.client + QLatin1Char(' ') + (article ? article->name : QString())
            + QLatin1Char(' ') + m_store.lotText(entry) + QLatin1Char(' ') + statusLabel(entry.status);
        if (!query.isEmpty() && !searchable.contains(query, Qt::CaseInsensitive))
            continue;
        m_filteredIds.append(entry.id);
    }
    std::sort(m_filteredIds.begin(), m_filteredIds.end(), [this](int left, int right) {
        const Order &a = *m_store.order(left);
        const Order &b = *m_store.order(right);
        if (a.date != b.date) {
            return m_sortMode == SortMode::DateNewest
                ? a.date > b.date
                : a.date < b.date;
        }
        // Keep a deterministic order when two orders have the same date.
        return a.id > b.id;
    });
    const int total = static_cast<int>(m_filteredIds.size());
    const int lastPage = std::max(1, (total + PageSize - 1) / PageSize);
    m_page = std::clamp(m_page, 1, lastPage);
    const int start = (m_page - 1) * PageSize;
    const int visible = std::min(PageSize, total - start);
    m_table->setUpdatesEnabled(false);
    m_table->clearContents();
    m_table->setRowCount(visible);
    for (int row = 0; row < visible; ++row) {
        const Order &entry = *m_store.order(m_filteredIds.at(start + row));
        const Product *article = m_store.product(entry.productId);
        const QStringList values = {
            QString::number(entry.id), entry.date.toString(QStringLiteral("dd/MM/yyyy")), entry.client,
            article ? article->name : QStringLiteral("-"),
            formatQuantity(entry.quantity) + QLatin1Char(' ') + (article ? article->unit : QString()),
            m_store.lotText(entry), formatMoney(m_store.amountCents(entry)), statusLabel(entry.status)
        };
        for (int column = 0; column < 8; ++column) {
            auto *item = new QTableWidgetItem(values.at(column));
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            item->setToolTip(column == 5 ? m_store.lotText(entry, true) : values.at(column));
            if (column == 0)
                item->setData(Qt::UserRole, entry.id);
            else if (column == 7)
                item->setData(Qt::UserRole, static_cast<int>(entry.status));
            m_table->setItem(row, column, item);
        }
        auto *actions = new QWidget(m_table);
        auto *actionsLayout = new QHBoxLayout(actions);
        actionsLayout->setContentsMargins(1, 0, 1, 0);
        actionsLayout->setSpacing(1);
        actionsLayout->setAlignment(Qt::AlignCenter);
        auto *edit = new QToolButton(actions);
        edit->setProperty("action", QStringLiteral("edit"));
        edit->setIcon(makeIcon(IconKind::Edit, QColor(QStringLiteral("#2275ae")), 16));
        edit->setIconSize(QSize(16, 16));
        edit->setFixedSize(25, 27);
        edit->setToolTip(QStringLiteral("Modifier la commande"));
        edit->setAccessibleName(QStringLiteral("Modifier la commande %1").arg(entry.id));
        edit->setCursor(Qt::PointingHandCursor);
        auto *remove = new QToolButton(actions);
        remove->setProperty("action", QStringLiteral("delete"));
        remove->setIcon(makeIcon(IconKind::Delete, Red, 16));
        remove->setIconSize(QSize(16, 16));
        remove->setFixedSize(25, 27);
        remove->setToolTip(QStringLiteral("Supprimer la commande"));
        remove->setAccessibleName(QStringLiteral("Supprimer la commande %1").arg(entry.id));
        remove->setCursor(Qt::PointingHandCursor);
        const int id = entry.id; // Capture a stable ID, not the visible row number.
        connect(edit, &QToolButton::clicked, this, [this, id] { editOrder(id); });
        connect(remove, &QToolButton::clicked, this, [this, id] { deleteOrder(id); });
        actionsLayout->addWidget(edit);
        actionsLayout->addWidget(remove);
        m_table->setCellWidget(row, 8, actions);
    }
    m_emptyLabel->setGeometry(m_table->viewport()->rect());
    m_emptyLabel->setVisible(total == 0);
    m_table->setUpdatesEnabled(true);
    m_exportButton->setEnabled(total > 0);
    updatePagination();
}

void MainWindow::updatePagination()
{
    while (QLayoutItem *item = m_pageButtonsLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
    const int total = static_cast<int>(m_filteredIds.size());
    const int last = std::max(1, (total + PageSize - 1) / PageSize);
    const int from = total == 0 ? 0 : (m_page - 1) * PageSize + 1;
    const int to = std::min(m_page * PageSize, total);
    m_pageLabel->setText(QStringLiteral("Affichage de %1 à %2 sur %3 commandes").arg(from).arg(to).arg(total));
    auto add = [&](const QString &text, int target, const QIcon &icon = QIcon()) -> QPushButton * {
        auto *button = new QPushButton(text, this);
        button->setProperty("pagination", true);
        button->setFixedSize(32, 32);
        button->setCursor(Qt::PointingHandCursor);
        if (!icon.isNull()) {
            button->setIcon(icon);
            button->setIconSize(QSize(16, 16));
        }
        connect(button, &QPushButton::clicked, this, [this, target] {
            m_page = target;
            refreshTable();
        });
        m_pageButtonsLayout->addWidget(button);
        return button;
    };
    auto *previous = add(QString(), m_page - 1, makeIcon(IconKind::Previous, QColor(QStringLiteral("#263b53")), 16));
    previous->setEnabled(m_page > 1);
    previous->setToolTip(QStringLiteral("Page précédente"));
    const int firstButton = std::max(1, std::min(m_page - 2, last - 4));
    const int lastButton = std::min(last, firstButton + 4);
    for (int page = firstButton; page <= lastButton; ++page) {
        auto *button = add(QString::number(page), page);
        button->setProperty("active", page == m_page);
        button->setToolTip(QStringLiteral("Page %1").arg(page));
    }
    auto *next = add(QString(), m_page + 1, makeIcon(IconKind::Next, QColor(QStringLiteral("#263b53")), 16));
    next->setEnabled(m_page < last);
    next->setToolTip(QStringLiteral("Page suivante"));
}

void MainWindow::updateCharts()
{
    const Product *article = m_store.product(m_articleCombo->currentData().toInt());
    if (!article)
        return;
    // Do not add L, kg and pcs together. The first two charts follow the
    // selected article, independently of the order table's search filters.
    QMap<QString, double> clients;
    QMap<QString, double> months;
    for (const Order &entry : m_store.orders()) {
        if (entry.status != OrderStatus::Delivered || entry.productId != article->id)
            continue;
        clients[entry.client] += entry.quantity;
        months[entry.date.toString(QStringLiteral("yyyy-MM"))] += entry.quantity;
    }
    QVector<QPair<QString, double>> ranked;
    for (auto it = clients.cbegin(); it != clients.cend(); ++it)
        ranked.append(qMakePair(it.key(), it.value()));
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
        return a.second == b.second ? a.first < b.first : a.second > b.second;
    });
    const std::array<QColor, 5> palette = {Blue, Green, Amber, QColor(QStringLiteral("#7785c4")),
                                         QColor(QStringLiteral("#3daab1"))};
    QVector<ChartBar> clientBars;
    const int clientCount = std::min(5, static_cast<int>(ranked.size()));
    for (int index = 0; index < clientCount; ++index)
        clientBars.append({ranked.at(index).first, ranked.at(index).second, palette[index], -1.0, article->unit});
    m_clientChart->setBars(clientBars);
    m_clientChartSubtitle->setText(article->name + QStringLiteral(" · ") + article->unit
                                    + (ranked.size() > 5 ? QStringLiteral(" · Top 5") : QString()));
    m_clientChartSubtitle->setToolTip(m_clientChartSubtitle->text());

    QVector<ChartBar> monthBars;
    const QStringList keys = months.keys();
    const int start = std::max(0, static_cast<int>(keys.size()) - 6);
    for (int index = start; index < static_cast<int>(keys.size()); ++index) {
        const QString &key = keys.at(index);
        const QString label = QDate::fromString(key + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd"))
                                  .toString(QStringLiteral("MM/yy"));
        monthBars.append({label, months.value(key), palette[(index - start) % 5], -1.0, article->unit});
    }
    m_monthChart->setBars(monthBars);
    m_monthChartSubtitle->setText(article->name + QStringLiteral(" · ") + article->unit);
    m_monthChartSubtitle->setToolTip(QStringLiteral("Six derniers mois comportant une livraison pour l'article choisi."));
    QVector<ChartBar> stockBars;
    for (const Product &item : m_store.products()) {
        const double stock = m_store.availableStock(item.id);
        stockBars.append({item.shortName + QStringLiteral(" (%1)").arg(item.unit), stock,
                          stock < item.minimumStock ? Red : Green, item.minimumStock, item.unit});
    }
    m_stockChart->setBars(stockBars);
}

void MainWindow::updateDiagnostics()
{
    QStringList lines;
    for (const Product &article : m_store.products()) {
        const double stock = m_store.availableStock(article.id);
        if (stock < article.minimumStock)
            lines.append(QStringLiteral("Stock sous le seuil : %1 (%2 %3).")
                             .arg(article.name.toHtmlEscaped(), formatQuantity(stock), article.unit));
    }
    for (const Lot &lot : m_store.lots()) {
        const qint64 days = QDate::currentDate().daysTo(lot.expiry);
        if (!lot.blocked && days >= 0 && days <= 30 && m_store.lotRemaining(lot.code) > 0.0)
            lines.append(QStringLiteral("Expiration sous 30 jours : %1 (%2 j).")
                             .arg(lot.code.toHtmlEscaped()).arg(days));
    }
    if (lines.isEmpty())
        lines.append(QStringLiteral("Aucune alerte de stock ou d'expiration."));
    lines.append(QStringLiteral("FEFO : le lot qui expire en premier est utilisé. Les lots bloqués ou périmés sont exclus."));
    m_diagnostics->setText(QStringLiteral("<b style='font-size:14px'>Diagnostic automatique</b><br>")
                            + lines.join(QStringLiteral("<br>")));
}

void MainWindow::clearForm()
{
    m_editingId = 0;
    m_formTitle->setText(QStringLiteral("Ajouter une nouvelle\ncommande"));
    m_saveButton->setText(QStringLiteral("+ Ajouter"));
    m_clearButton->setText(QStringLiteral("Vider"));
    m_clientEdit->clear();
    m_dateEdit->setDate(QDate::currentDate());
    m_articleCombo->setCurrentIndex(0);
    m_statusCombo->setCurrentIndex(0);
    m_quantityEdit->setDecimals(2);
    m_quantityEdit->setSuffix(QStringLiteral(" L"));
    m_quantityEdit->setValue(0.0);
    m_feedbackLabel->clear();
    updateDerivedFields();
    updateCharts();
}

void MainWindow::saveOrder()
{
    // Commit typed spin-box text even when keyboard tracking is disabled.
    m_quantityEdit->interpretText();
    Order candidate = orderFromForm();
    QString error;
    if (!m_store.saveOrder(candidate, &error)) {
        QMessageBox::warning(this, QStringLiteral("Commande non enregistrée"), error);
        updateDerivedFields();
        return;
    }
    const int savedId = candidate.id;
    updateClientChoices();
    clearForm();
    m_page = 1;
    refreshTable();
    updateCharts();
    updateDiagnostics();
    m_feedbackLabel->setText(QStringLiteral("Commande #%1 enregistrée.").arg(savedId));
}

void MainWindow::editOrder(int id)
{
    const Order *entry = m_store.order(id);
    if (!entry)
        return;
    // Copy before touching controls: avoids reliance on model pointer lifetime.
    const Order copy = *entry;
    m_editingId = copy.id;
    m_formTitle->setText(QStringLiteral("Modifier la\ncommande #%1").arg(copy.id));
    m_saveButton->setText(QStringLiteral("Enregistrer"));
    m_clearButton->setText(QStringLiteral("Annuler"));
    m_dateEdit->setDate(copy.date);
    m_clientEdit->setText(copy.client);
    m_articleCombo->setCurrentIndex(m_articleCombo->findData(copy.productId));
    m_quantityEdit->setValue(copy.quantity);
    m_statusCombo->setCurrentIndex(m_statusCombo->findData(static_cast<int>(copy.status)));
    m_feedbackLabel->setText(QStringLiteral("Modification de la commande #%1.").arg(copy.id));
    m_clientEdit->setFocus();
    updateDerivedFields();
    updateCharts();
}

void MainWindow::deleteOrder(int id)
{
    const Order *entry = m_store.order(id);
    if (!entry)
        return;
    const auto answer = QMessageBox::question(this, QStringLiteral("Supprimer une commande"),
        QStringLiteral("Supprimer la commande #%1 de %2 ?\n\n"
                       "Dans cette démo, son allocation de stock sera libérée.")
            .arg(id).arg(entry->client), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    m_store.removeOrder(id);
    if (m_editingId == id)
        clearForm();
    updateClientChoices();
    refreshTable();
    updateDerivedFields();
    updateCharts();
    updateDiagnostics();
    m_feedbackLabel->setText(QStringLiteral("Commande #%1 supprimée.").arg(id));
}

void MainWindow::exportPdf()
{
    QString filename = QFileDialog::getSaveFileName(this, QStringLiteral("Exporter les commandes"),
        QDir::homePath() + QStringLiteral("/commandes_%1.pdf").arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))),
        QStringLiteral("Documents PDF (*.pdf)"));
    if (filename.isEmpty())
        return;
    if (!filename.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
        filename += QStringLiteral(".pdf");
        if (QFileInfo::exists(filename)
            && QMessageBox::question(this, QStringLiteral("Remplacer le fichier ?"),
                QStringLiteral("Le fichier existe :\n%1\n\nLe remplacer ?").arg(filename),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
            return;
    }
    QVector<Order> result;
    for (int id : m_filteredIds)
        if (const Order *entry = m_store.order(id))
            result.append(*entry);
    const QString context = QStringLiteral("Filtres : %1 | %2 | Recherche : %3")
        .arg(m_statusFilter->currentText(), m_clientFilter->currentText(),
             m_searchEdit->text().trimmed().isEmpty() ? QStringLiteral("aucune") : m_searchEdit->text().trimmed());
    QString error;
    if (!exportOrdersPdf(filename, result, m_store, context, &error)) {
        QMessageBox::critical(this, QStringLiteral("Export impossible"), error);
        return;
    }
    QMessageBox::information(this, QStringLiteral("Export terminé"),
        QStringLiteral("%1 commande(s) exportée(s) dans :\n%2").arg(result.size()).arg(QDir::toNativeSeparators(filename)));
}
