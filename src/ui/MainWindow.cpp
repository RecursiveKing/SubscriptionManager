#include "MainWindow.h"
#include "SubscriptionDialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// Вспомогательная функция: собирает красивую карточку для верхней панели
QFrame* buildCard(const QString& title, QLabel*& valueLabel, QWidget* parent) {
    auto* card = new QFrame(parent);
    card->setFrameShape(QFrame::StyledPanel);
    card->setStyleSheet(
        "QFrame { background-color: #f8f9fa; border: 1px solid #dcdcdc; border-radius: 8px; padding: 8px; }"
    );

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(4);

    auto* titleLbl = new QLabel(title, card);
    titleLbl->setStyleSheet("color: #666666; font-size: 12px; border: none;");

    valueLabel = new QLabel("—", card);
    valueLabel->setStyleSheet("color: #1f2937; font-size: 18px; font-weight: bold; border: none;");

    layout->addWidget(titleLbl);
    layout->addWidget(valueLabel);
    return card;
}

// Форматирует суммы по валютам (BYN всегда выводим первым)
QString formatCurrencyTotals(const QMap<QString, double>& totals) {
    if (totals.isEmpty()) {
        return "0.00 BYN";
    }

    QStringList parts;
    if (totals.contains("BYN")) {
        parts << QString("%1 BYN").arg(totals.value("BYN"), 0, 'f', 2);
    }
    for (auto it = totals.cbegin(); it != totals.cend(); ++it) {
        if (it.key() != "BYN") {
            parts << QString("%1 %2").arg(it.value(), 0, 'f', 2).arg(it.key());
        }
    }
    return parts.join(" + ");
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Менеджер подписок и регулярных платежей");
    resize(1040, 660);

    const QString dbPath = QCoreApplication::applicationDirPath() + "/subscriptions.db";
    if (!m_db.init(dbPath)) {
        QMessageBox::critical(this, "Ошибка базы данных",
                              "Не удалось открыть базу данных:\n" + m_db.lastError());
    }

    setupUi();
    reloadAllData();
    statusBar()->showMessage("Готово к работе. База данных: " + dbPath);
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(14, 14, 14, 10);

    mainLayout->addWidget(createSummaryBar());

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createSubscriptionsTab(), "Подписки");
    m_tabs->addTab(createHistoryTab(), "История платежей");
    mainLayout->addWidget(m_tabs, 1);

    setCentralWidget(central);
}

QWidget* MainWindow::createSummaryBar() {
    auto* container = new QWidget(this);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    layout->addWidget(buildCard("Расходы в месяц", m_monthlyTotalLabel, container), 1);
    layout->addWidget(buildCard("Расходы в год", m_yearlyTotalLabel, container), 1);
    layout->addWidget(buildCard("Ближайшее списание", m_nextPaymentLabel, container), 1);

    return container;
}

QWidget* MainWindow::createSubscriptionsTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);
    layout->setSpacing(10);

    // Панель поиска и фильтров
    auto* filterLayout = new QHBoxLayout();

    m_searchEdit = new QLineEdit(tab);
    m_searchEdit->setPlaceholderText("Поиск по названию или заметкам...");
    m_searchEdit->setClearButtonEnabled(true);

    m_categoryFilter = new QComboBox(tab);
    m_categoryFilter->setMinimumWidth(180);
    m_categoryFilter->addItem("Все категории");

    m_onlyActiveCheck = new QCheckBox("Только активные", tab);

    filterLayout->addWidget(m_searchEdit, 1);
    filterLayout->addWidget(new QLabel("Категория:", tab));
    filterLayout->addWidget(m_categoryFilter);
    filterLayout->addWidget(m_onlyActiveCheck);
    layout->addLayout(filterLayout);

    // Таблица подписок
    m_subsTable = new QTableWidget(tab);
    m_subsTable->setColumnCount(7);
    m_subsTable->setHorizontalHeaderLabels({
        "Название", "Категория", "Сумма", "Периодичность",
        "В месяц", "Следующее списание", "Статус"
    });
    m_subsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_subsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_subsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_subsTable->setAlternatingRowColors(true);
    m_subsTable->verticalHeader()->setVisible(false);

    auto* header = m_subsTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    layout->addWidget(m_subsTable, 1);

    // Панель кнопок действий
    auto* btnLayout = new QHBoxLayout();
    m_addBtn = new QPushButton("+ Добавить", tab);
    m_editBtn = new QPushButton("Редактировать", tab);
    m_paidBtn = new QPushButton("Оплачено", tab);
    m_deleteBtn = new QPushButton("Удалить", tab);

    m_addBtn->setStyleSheet("padding: 6px 14px; font-weight: bold;");
    m_paidBtn->setStyleSheet("padding: 6px 14px;");
    m_editBtn->setStyleSheet("padding: 6px 14px;");
    m_deleteBtn->setStyleSheet("padding: 6px 14px;");

    btnLayout->addWidget(m_addBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_paidBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_deleteBtn);
    layout->addLayout(btnLayout);

    // Подключение сигналов
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddSubscription);
    connect(m_editBtn, &QPushButton::clicked, this, &MainWindow::onEditSubscription);
    connect(m_paidBtn, &QPushButton::clicked, this, &MainWindow::onMarkAsPaid);
    connect(m_deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteSubscription);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::applyFilters);
    connect(m_categoryFilter, &QComboBox::currentIndexChanged, this, &MainWindow::applyFilters);
    connect(m_onlyActiveCheck, &QCheckBox::toggled, this, &MainWindow::applyFilters);

    connect(m_subsTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::updateButtonsState);
    connect(m_subsTable, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { onEditSubscription(); });

    return tab;
}

QWidget* MainWindow::createHistoryTab() {
    auto* tab = new QWidget(this);
    auto* layout = new QVBoxLayout(tab);

    m_historyTable = new QTableWidget(tab);
    m_historyTable->setColumnCount(3);
    m_historyTable->setHorizontalHeaderLabels({"Дата оплаты", "Подписка", "Сумма"});
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->verticalHeader()->setVisible(false);

    auto* header = m_historyTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    layout->addWidget(m_historyTable);
    return tab;
}

void MainWindow::reloadAllData() {
    m_subscriptions = m_db.getAllSubscriptions();
    refreshCategoryFilter();
    populateSubscriptionsTable();
    populateHistoryTable();
    updateSummaryCards();
    updateButtonsState();
}

void MainWindow::refreshCategoryFilter() {
    const QString previous = m_categoryFilter->currentText();

    // QSignalBlocker временно отключает сигналы комбобокса, чтобы не перерисовывать таблицу зря
    QSignalBlocker blocker(m_categoryFilter);
    m_categoryFilter->clear();
    m_categoryFilter->addItem("Все категории");

    QSet<QString> categories;
    for (const auto& sub : m_subscriptions) {
        categories.insert(sub.category);
    }

    QStringList sorted = categories.values();
    sorted.sort(Qt::CaseInsensitive);
    m_categoryFilter->addItems(sorted);

    int idx = m_categoryFilter->findText(previous);
    m_categoryFilter->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::populateSubscriptionsTable() {
    const QString search = m_searchEdit->text().trimmed().toLower();
    const QString selectedCategory = m_categoryFilter->currentText();
    const bool filterByCategory = (m_categoryFilter->currentIndex() > 0);
    const bool onlyActive = m_onlyActiveCheck->isChecked();

    m_subsTable->setRowCount(0);

    for (const auto& sub : m_subscriptions) {
        if (onlyActive && !sub.active) {
            continue;
        }
        if (filterByCategory && sub.category != selectedCategory) {
            continue;
        }
        if (!search.isEmpty() &&
            !sub.name.toLower().contains(search) &&
            !sub.notes.toLower().contains(search)) {
            continue;
        }

        int row = m_subsTable->rowCount();
        m_subsTable->insertRow(row);

        // 0. Название (прячем ID подписки в Qt::UserRole)
        auto* nameItem = new QTableWidgetItem(sub.name);
        nameItem->setData(Qt::UserRole, sub.id);
        if (!sub.notes.isEmpty()) {
            nameItem->setToolTip("Заметка: " + sub.notes);
        }

        // 1. Категория
        auto* catItem = new QTableWidgetItem(sub.category);

        // 2. Сумма
        auto* amountItem = new QTableWidgetItem(
            QString("%1 %2").arg(sub.amount, 0, 'f', 2).arg(sub.currency));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // 3. Периодичность
        QString cycleText = (sub.cycle == BillingCycle::CustomDays)
            ? QString("Каждые %1 дн.").arg(sub.customDays)
            : Subscription::cycleToDisplayName(sub.cycle);
        auto* cycleItem = new QTableWidgetItem(cycleText);

        // 4. Стоимость в месяц
        auto* monthlyItem = new QTableWidgetItem(
            QString("%1 %2").arg(sub.monthlyCost(), 0, 'f', 2).arg(sub.currency));
        monthlyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // 5. Следующее списание и сколько дней осталось
        int daysLeft = sub.daysUntilPayment();
        QString dateText = sub.nextPaymentDate.toString("dd.MM.yyyy");
        if (sub.active) {
            if (daysLeft < 0) {
                dateText += QString(" (просрочено %1 дн.)").arg(-daysLeft);
            } else if (daysLeft == 0) {
                dateText += " (сегодня!)";
            } else if (daysLeft <= 7) {
                dateText += QString(" (через %1 дн.)").arg(daysLeft);
            }
        }
        auto* dateItem = new QTableWidgetItem(dateText);

        // 6. Статус
        auto* statusItem = new QTableWidgetItem(sub.active ? "Активна" : "На паузе");
        statusItem->setTextAlignment(Qt::AlignCenter);

        QList<QTableWidgetItem*> items = {
            nameItem, catItem, amountItem, cycleItem, monthlyItem, dateItem, statusItem
        };

        // Цветовая индикация строки
        for (int col = 0; col < items.size(); ++col) {
            if (!sub.active) {
                items[col]->setForeground(QColor("#888888"));
            } else if (daysLeft < 0) {
                items[col]->setBackground(QColor("#ffe4e6"));
                items[col]->setForeground(QColor("#9f1239"));
            } else if (daysLeft <= 3) {
                items[col]->setBackground(QColor("#fef3c7"));
                items[col]->setForeground(QColor("#92400e"));
            }
            m_subsTable->setItem(row, col, items[col]);
        }
    }
}

void MainWindow::populateHistoryTable() {
    const auto history = m_db.getPaymentHistory();
    m_historyTable->setRowCount(0);

    for (const auto& rec : history) {
        int row = m_historyTable->rowCount();
        m_historyTable->insertRow(row);

        auto* dateItem = new QTableWidgetItem(rec.paidDate.toString("dd.MM.yyyy"));
        auto* nameItem = new QTableWidgetItem(rec.subscriptionName);
        auto* amountItem = new QTableWidgetItem(
            QString("%1 %2").arg(rec.amount, 0, 'f', 2).arg(rec.currency));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_historyTable->setItem(row, 0, dateItem);
        m_historyTable->setItem(row, 1, nameItem);
        m_historyTable->setItem(row, 2, amountItem);
    }
}

void MainWindow::updateSummaryCards() {
    QMap<QString, double> monthlyTotals;
    QMap<QString, double> yearlyTotals;

    const Subscription* nearest = nullptr;

    for (const auto& sub : m_subscriptions) {
        if (!sub.active) {
            continue;
        }
        monthlyTotals[sub.currency] += sub.monthlyCost();
        yearlyTotals[sub.currency] += sub.yearlyCost();

        if (nearest == nullptr || sub.nextPaymentDate < nearest->nextPaymentDate) {
            nearest = &sub;
        }
    }

    m_monthlyTotalLabel->setText(formatCurrencyTotals(monthlyTotals));
    m_yearlyTotalLabel->setText(formatCurrencyTotals(yearlyTotals));

    if (nearest != nullptr) {
        m_nextPaymentLabel->setText(
            QString("%1 — %2")
                .arg(nearest->name, nearest->nextPaymentDate.toString("dd.MM.yyyy")));
    } else {
        m_nextPaymentLabel->setText("Нет активных");
    }
}

int MainWindow::selectedSubscriptionId() const {
    int row = m_subsTable->currentRow();
    if (row < 0) {
        return -1;
    }
    auto* item = m_subsTable->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

const Subscription* MainWindow::findSubscriptionById(int id) const {
    for (const auto& sub : m_subscriptions) {
        if (sub.id == id) {
            return &sub;
        }
    }
    return nullptr;
}

void MainWindow::updateButtonsState() {
    const Subscription* selected = findSubscriptionById(selectedSubscriptionId());
    bool hasSelection = (selected != nullptr);

    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
    m_paidBtn->setEnabled(hasSelection && selected->active);
}

void MainWindow::applyFilters() {
    populateSubscriptionsTable();
    updateButtonsState();
}

void MainWindow::onAddSubscription() {
    SubscriptionDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Subscription sub = dialog.subscription();
        if (m_db.addSubscription(sub)) {
            reloadAllData();
            statusBar()->showMessage("Добавлена подписка: " + sub.name, 4000);
        } else {
            QMessageBox::critical(this, "Ошибка сохранения", m_db.lastError());
        }
    }
}

void MainWindow::onEditSubscription() {
    const Subscription* existing = findSubscriptionById(selectedSubscriptionId());
    if (!existing) {
        return;
    }

    SubscriptionDialog dialog(this, existing);
    if (dialog.exec() == QDialog::Accepted) {
        Subscription updated = dialog.subscription();
        if (m_db.updateSubscription(updated)) {
            reloadAllData();
            statusBar()->showMessage("Подписка обновлена: " + updated.name, 4000);
        } else {
            QMessageBox::critical(this, "Ошибка обновления", m_db.lastError());
        }
    }
}

void MainWindow::onDeleteSubscription() {
    const Subscription* existing = findSubscriptionById(selectedSubscriptionId());
    if (!existing) {
        return;
    }

    auto reply = QMessageBox::question(
        this,
        "Удаление подписки",
        QString("Удалить подписку «%1» и всю её историю платежей?").arg(existing->name),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        const QString deletedName = existing->name;
        if (m_db.deleteSubscription(existing->id)) {
            reloadAllData();
            statusBar()->showMessage("Подписка удалена: " + deletedName, 4000);
        } else {
            QMessageBox::critical(this, "Ошибка удаления", m_db.lastError());
        }
    }
}

void MainWindow::onMarkAsPaid() {
    const Subscription* existing = findSubscriptionById(selectedSubscriptionId());
    if (!existing) {
        return;
    }

    Subscription subCopy = *existing;
    if (m_db.markAsPaid(subCopy)) {
        reloadAllData();
        statusBar()->showMessage(
            QString("Оплата «%1» записана. Следующее списание: %2")
                .arg(subCopy.name, subCopy.nextPaymentDate.toString("dd.MM.yyyy")),
            5000);
    } else {
        QMessageBox::critical(this, "Ошибка записи платежа", m_db.lastError());
    }
}