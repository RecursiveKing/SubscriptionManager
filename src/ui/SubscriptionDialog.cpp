#include "SubscriptionDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

SubscriptionDialog::SubscriptionDialog(QWidget* parent, const Subscription* existing)
    : QDialog(parent)
{
    setupUi();

    if (existing != nullptr) {
        m_subscription = *existing;
        setWindowTitle("Редактировать подписку");
        fillFromSubscription(m_subscription);
    } else {
        setWindowTitle("Добавить подписку");
        m_subscription.currency = "BYN";
    }

    updateCustomDaysVisibility();
}

void SubscriptionDialog::setupUi() {
    setMinimumWidth(420);

    auto* mainLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    // Название сервиса или платежа
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Например: Яндекс Плюс, Интернет A1, Spotify");
    formLayout->addRow("Название:", m_nameEdit);

    // Категория (можно выбрать готовую или вписать свою)
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems({
        "Стриминг и музыка",
        "Интернет и связь",
        "Облако и софт",
        "Игры",
        "Коммунальные",
        "Спорт и здоровье",
        "Образование",
        "Другое"
    });
    formLayout->addRow("Категория:", m_categoryCombo);

    // Сумма и валюта в одной строке
    auto* amountLayout = new QHBoxLayout();
    m_amountSpin = new QDoubleSpinBox(this);
    m_amountSpin->setRange(0.01, 1000000.0);
    m_amountSpin->setDecimals(2);
    m_amountSpin->setSingleStep(1.0);
    m_amountSpin->setValue(10.00);

    m_currencyCombo = new QComboBox(this);
    m_currencyCombo->addItems({"BYN", "USD", "EUR", "RUB"});
    m_currencyCombo->setMaximumWidth(80);

    amountLayout->addWidget(m_amountSpin, 1);
    amountLayout->addWidget(m_currencyCombo);
    formLayout->addRow("Сумма:", amountLayout);

    // Периодичность списания
    m_cycleCombo = new QComboBox(this);
    m_cycleCombo->addItem(Subscription::cycleToDisplayName(BillingCycle::Weekly),
                          static_cast<int>(BillingCycle::Weekly));
    m_cycleCombo->addItem(Subscription::cycleToDisplayName(BillingCycle::Monthly),
                          static_cast<int>(BillingCycle::Monthly));
    m_cycleCombo->addItem(Subscription::cycleToDisplayName(BillingCycle::Quarterly),
                          static_cast<int>(BillingCycle::Quarterly));
    m_cycleCombo->addItem(Subscription::cycleToDisplayName(BillingCycle::Yearly),
                          static_cast<int>(BillingCycle::Yearly));
    m_cycleCombo->addItem(Subscription::cycleToDisplayName(BillingCycle::CustomDays),
                          static_cast<int>(BillingCycle::CustomDays));
    // По умолчанию выбираем "Ежемесячно"
    m_cycleCombo->setCurrentIndex(1);
    formLayout->addRow("Периодичность:", m_cycleCombo);

    // Поле для своего интервала в днях (показывается только при выборе CustomDays)
    m_customDaysLabel = new QLabel("Интервал (дней):", this);
    m_customDaysSpin = new QSpinBox(this);
    m_customDaysSpin->setRange(1, 3650);
    m_customDaysSpin->setValue(30);
    formLayout->addRow(m_customDaysLabel, m_customDaysSpin);

    // Дата следующего списания с выпадающим календарём
    m_nextDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_nextDateEdit->setCalendarPopup(true);
    m_nextDateEdit->setDisplayFormat("dd.MM.yyyy");
    formLayout->addRow("Следующее списание:", m_nextDateEdit);

    // Статус активности
    m_activeCheck = new QCheckBox("Подписка активна", this);
    m_activeCheck->setChecked(true);
    formLayout->addRow("", m_activeCheck);

    // Заметки
    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(75);
    m_notesEdit->setPlaceholderText("Дополнительные заметки (карта списания, тариф и т.д.)");
    formLayout->addRow("Заметки:", m_notesEdit);

    mainLayout->addLayout(formLayout);

    // Стандартные кнопки OK / Отмена
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttonBox->button(QDialogButtonBox::Ok)->setText("Сохранить");
    buttonBox->button(QDialogButtonBox::Cancel)->setText("Отмена");
    mainLayout->addWidget(buttonBox);

    // Подключаем сигналы к слотам
    connect(m_cycleCombo, &QComboBox::currentIndexChanged,
            this, &SubscriptionDialog::updateCustomDaysVisibility);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &SubscriptionDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

void SubscriptionDialog::fillFromSubscription(const Subscription& sub) {
    m_nameEdit->setText(sub.name);
    m_categoryCombo->setCurrentText(sub.category);
    m_amountSpin->setValue(sub.amount);

    int currIdx = m_currencyCombo->findText(sub.currency);
    if (currIdx >= 0) {
        m_currencyCombo->setCurrentIndex(currIdx);
    }

    int cycleIdx = m_cycleCombo->findData(static_cast<int>(sub.cycle));
    if (cycleIdx >= 0) {
        m_cycleCombo->setCurrentIndex(cycleIdx);
    }

    m_customDaysSpin->setValue(sub.customDays);
    m_nextDateEdit->setDate(sub.nextPaymentDate);
    m_activeCheck->setChecked(sub.active);
    m_notesEdit->setPlainText(sub.notes);
}

void SubscriptionDialog::updateCustomDaysVisibility() {
    auto selectedCycle = static_cast<BillingCycle>(m_cycleCombo->currentData().toInt());
    bool isCustom = (selectedCycle == BillingCycle::CustomDays);
    m_customDaysLabel->setVisible(isCustom);
    m_customDaysSpin->setVisible(isCustom);
}

void SubscriptionDialog::onAccept() {
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка ввода", "Введите название подписки.");
        m_nameEdit->setFocus();
        return;
    }

    const QString category = m_categoryCombo->currentText().trimmed();
    if (category.isEmpty()) {
        QMessageBox::warning(this, "Ошибка ввода", "Укажите категорию.");
        m_categoryCombo->setFocus();
        return;
    }

    m_subscription.name = name;
    m_subscription.category = category;
    m_subscription.amount = m_amountSpin->value();
    m_subscription.currency = m_currencyCombo->currentText();
    m_subscription.cycle = static_cast<BillingCycle>(m_cycleCombo->currentData().toInt());
    m_subscription.customDays = m_customDaysSpin->value();
    m_subscription.nextPaymentDate = m_nextDateEdit->date();
    m_subscription.active = m_activeCheck->isChecked();
    m_subscription.notes = m_notesEdit->toPlainText().trimmed();

    accept();
}