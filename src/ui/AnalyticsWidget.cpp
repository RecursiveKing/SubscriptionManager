#include "AnalyticsWidget.h"

#include <algorithm>

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QPair>
#include <QSet>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

AnalyticsWidget::AnalyticsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void AnalyticsWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Верхняя панель: выбор валюты для диаграммы и краткий итог
    auto* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("Валюта для анализа:", this));

    m_currencyCombo = new QComboBox(this);
    m_currencyCombo->setMinimumWidth(90);
    m_currencyCombo->addItem("BYN");
    topBar->addWidget(m_currencyCombo);

    topBar->addSpacing(16);

    m_totalInfoLabel = new QLabel("Нет активных подписок", this);
    m_totalInfoLabel->setStyleSheet("font-weight: bold; color: #333333;");
    topBar->addWidget(m_totalInfoLabel);
    topBar->addStretch();

    mainLayout->addLayout(topBar);

    // Основная область: слева круговая диаграмма, справа таблица по категориям
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(14);

    m_chart = new QChart();
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    m_chart->setBackgroundRoundness(8);

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumWidth(420);

    m_breakdownTable = new QTableWidget(this);
    m_breakdownTable->setColumnCount(4);
    m_breakdownTable->setHorizontalHeaderLabels({"Категория", "В месяц", "В год", "Доля"});
    m_breakdownTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_breakdownTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_breakdownTable->setAlternatingRowColors(true);
    m_breakdownTable->verticalHeader()->setVisible(false);

    auto* header = m_breakdownTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    contentLayout->addWidget(m_chartView, 3);
    contentLayout->addWidget(m_breakdownTable, 2);

    mainLayout->addLayout(contentLayout, 1);

    connect(m_currencyCombo, &QComboBox::currentIndexChanged,
            this, &AnalyticsWidget::rebuildChart);
}

void AnalyticsWidget::updateData(const QVector<Subscription>& subscriptions) {
    m_subscriptions = subscriptions;

    const QString prevCurrency = m_currencyCombo->currentText();

    // Собираем уникальные валюты среди активных подписок
    QSet<QString> currencies;
    for (const auto& sub : m_subscriptions) {
        if (sub.active) {
            currencies.insert(sub.currency);
        }
    }

    QSignalBlocker blocker(m_currencyCombo);
    m_currencyCombo->clear();

    if (currencies.isEmpty()) {
        m_currencyCombo->addItem("BYN");
    } else {
        // Ставим BYN первым в списке, если он есть
        if (currencies.contains("BYN")) {
            m_currencyCombo->addItem("BYN");
            currencies.remove("BYN");
        }
        QStringList others = currencies.values();
        others.sort();
        m_currencyCombo->addItems(others);
    }

    int idx = m_currencyCombo->findText(prevCurrency);
    m_currencyCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    rebuildChart();
}

void AnalyticsWidget::rebuildChart() {
    const QString currency = m_currencyCombo->currentText();

    // Группируем ежемесячные расходы по категориям
    QMap<QString, double> monthlyByCategory;
    double totalMonthly = 0.0;

    for (const auto& sub : m_subscriptions) {
        if (!sub.active || sub.currency != currency) {
            continue;
        }
        const double cost = sub.monthlyCost();
        monthlyByCategory[sub.category] += cost;
        totalMonthly += cost;
    }

    // Очищаем старый график (removeAllSeries сам удалит прошлую QPieSeries из памяти)
    m_chart->removeAllSeries();
    m_breakdownTable->setRowCount(0);

    if (monthlyByCategory.isEmpty() || totalMonthly <= 0.0) {
        m_chart->setTitle(QString("Нет активных подписок в валюте %1").arg(currency));
        m_totalInfoLabel->setText("Итого: 0.00 " + currency + " / мес");
        return;
    }

    // Перекладываем в вектор и сортируем категории от самой дорогой к самой дешёвой
    QVector<QPair<QString, double>> sorted;
    sorted.reserve(monthlyByCategory.size());
    for (auto it = monthlyByCategory.cbegin(); it != monthlyByCategory.cend(); ++it) {
        sorted.push_back({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Палитра цветов для секторов диаграммы
    const QList<QColor> palette = {
        QColor("#3b82f6"), QColor("#10b981"), QColor("#f59e0b"),
        QColor("#ef4444"), QColor("#8b5cf6"), QColor("#ec4899"),
        QColor("#14b8a6"), QColor("#f97316")
    };

    auto* series = new QPieSeries();
    series->setHoleSize(0.35); // Делаем кольцевую диаграмму (Donut chart)

    for (int i = 0; i < sorted.size(); ++i) {
        const QString& category = sorted[i].first;
        const double monthly = sorted[i].second;
        const double yearly = monthly * 12.0;
        const double share = (monthly / totalMonthly) * 100.0;

        // Добавляем сектор на график
        QPieSlice* slice = series->append(category, monthly);
        slice->setColor(palette[i % palette.size()]);
        slice->setLabel(QString("%1 (%2%)").arg(category).arg(share, 0, 'f', 1));
        slice->setLabelVisible(share >= 6.0); // Подписываем крупные сектора, чтобы текст не налезал

        // Заполняем строку в таблице справа
        int row = m_breakdownTable->rowCount();
        m_breakdownTable->insertRow(row);

        auto* catItem = new QTableWidgetItem(category);
        auto* monthItem = new QTableWidgetItem(
            QString("%1 %2").arg(monthly, 0, 'f', 2).arg(currency));
        auto* yearItem = new QTableWidgetItem(
            QString("%1 %2").arg(yearly, 0, 'f', 2).arg(currency));
        auto* shareItem = new QTableWidgetItem(QString("%1%").arg(share, 0, 'f', 1));

        monthItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        yearItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        shareItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_breakdownTable->setItem(row, 0, catItem);
        m_breakdownTable->setItem(row, 1, monthItem);
        m_breakdownTable->setItem(row, 2, yearItem);
        m_breakdownTable->setItem(row, 3, shareItem);
    }

    m_chart->addSeries(series);
    m_chart->setTitle(QString("Расходы по категориям в месяц (%1)").arg(currency));

    m_totalInfoLabel->setText(
        QString("Итого: %1 %3 / мес  (%2 %3 / год)")
            .arg(totalMonthly, 0, 'f', 2)
            .arg(totalMonthly * 12.0, 0, 'f', 2)
            .arg(currency));
}