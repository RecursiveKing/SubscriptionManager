#pragma once

#include <QVector>
#include <QWidget>

#include "models/Subscription.h"

class QChart;
class QChartView;
class QComboBox;
class QLabel;
class QTableWidget;

class AnalyticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnalyticsWidget(QWidget* parent = nullptr);

    void updateData(const QVector<Subscription>& subscriptions);

private slots:
    void rebuildChart();

private:
    void setupUi();

    QVector<Subscription> m_subscriptions;

    QComboBox* m_currencyCombo = nullptr;
    QLabel* m_totalInfoLabel = nullptr;
    QChart* m_chart = nullptr;
    QChartView* m_chartView = nullptr;
    QTableWidget* m_breakdownTable = nullptr;
};