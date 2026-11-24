#pragma once

#include <QMainWindow>
#include <QVector>

#include "database/DatabaseManager.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onAddSubscription();
    void onEditSubscription();
    void onDeleteSubscription();
    void onMarkAsPaid();
    void applyFilters();
    void updateButtonsState();

private:
    void setupUi();
    QWidget* createSummaryBar();
    QWidget* createSubscriptionsTab();
    QWidget* createHistoryTab();

    void reloadAllData();
    void refreshCategoryFilter();
    void populateSubscriptionsTable();
    void populateHistoryTable();
    void updateSummaryCards();

    int selectedSubscriptionId() const;
    const Subscription* findSubscriptionById(int id) const;

    DatabaseManager m_db;
    QVector<Subscription> m_subscriptions;

    // Карточки сводки
    QLabel* m_monthlyTotalLabel = nullptr;
    QLabel* m_yearlyTotalLabel = nullptr;
    QLabel* m_nextPaymentLabel = nullptr;

    // Вкладки
    QTabWidget* m_tabs = nullptr;

    // Фильтры и таблица подписок
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_categoryFilter = nullptr;
    QCheckBox* m_onlyActiveCheck = nullptr;
    QTableWidget* m_subsTable = nullptr;

    // Кнопки действий
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_editBtn = nullptr;
    QPushButton* m_paidBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;

    // Таблица истории платежей
    QTableWidget* m_historyTable = nullptr;
};