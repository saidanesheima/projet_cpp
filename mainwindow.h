#pragma once

#include "salesstore.h"
#include <QMainWindow>

class BarChartWidget;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QStringListModel;
class QTableWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    SalesStore m_store;
    int m_editingId = 0;
    int m_page = 1;
    static constexpr int PageSize = 5;
    QVector<int> m_filteredIds;
    enum class SortMode { DateNewest, DateOldest };
    SortMode m_sortMode = SortMode::DateNewest;

    QLabel *m_formTitle = nullptr;
    QDateEdit *m_dateEdit = nullptr;
    QLineEdit *m_clientEdit = nullptr;
    QStringListModel *m_clientsModel = nullptr;
    QComboBox *m_articleCombo = nullptr;
    QDoubleSpinBox *m_quantityEdit = nullptr;
    QComboBox *m_statusCombo = nullptr;
    QLineEdit *m_stockEdit = nullptr;
    QLineEdit *m_lotEdit = nullptr;
    QLineEdit *m_amountEdit = nullptr;
    QLabel *m_controlBadge = nullptr;
    QLabel *m_diagnostics = nullptr;
    QLabel *m_feedbackLabel = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_clearButton = nullptr;

    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_statusFilter = nullptr;
    QComboBox *m_clientFilter = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QLabel *m_pageLabel = nullptr;
    QHBoxLayout *m_pageButtonsLayout = nullptr;
    QPushButton *m_exportButton = nullptr;

    BarChartWidget *m_clientChart = nullptr;
    BarChartWidget *m_monthChart = nullptr;
    BarChartWidget *m_stockChart = nullptr;
    QLabel *m_clientChartSubtitle = nullptr;
    QLabel *m_monthChartSubtitle = nullptr;

    QFrame *createHeader();
    QFrame *createFormPanel();
    QFrame *createOrdersPanel();
    QFrame *createStatisticsPanel();
    void connectSignals();
    void updateDerivedFields();
    void updateClientChoices();
    void refreshTable();
    void updatePagination();
    void updateCharts();
    void updateDiagnostics();
    void clearForm();
    void saveOrder();
    void editOrder(int id);
    void deleteOrder(int id);
    void exportPdf();
    Order orderFromForm() const;
    void setControlState(const QString &text, const QString &state, const QString &tooltip = QString());
};
