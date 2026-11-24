#pragma once

#include <QDialog>
#include "models/Subscription.h"

class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QDateEdit;
class QCheckBox;
class QTextEdit;
class QLabel;

class SubscriptionDialog : public QDialog {
    Q_OBJECT

public:
    explicit SubscriptionDialog(QWidget* parent = nullptr,
                                const Subscription* existing = nullptr);

    Subscription subscription() const { return m_subscription; }

private slots:
    void updateCustomDaysVisibility();
    void onAccept();

private:
    void setupUi();
    void fillFromSubscription(const Subscription& sub);

    Subscription m_subscription;

    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_categoryCombo = nullptr;
    QDoubleSpinBox* m_amountSpin = nullptr;
    QComboBox* m_currencyCombo = nullptr;
    QComboBox* m_cycleCombo = nullptr;
    QLabel* m_customDaysLabel = nullptr;
    QSpinBox* m_customDaysSpin = nullptr;
    QDateEdit* m_nextDateEdit = nullptr;
    QCheckBox* m_activeCheck = nullptr;
    QTextEdit* m_notesEdit = nullptr;
};