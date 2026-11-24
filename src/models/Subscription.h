#pragma once

#include <QDate>
#include <QString>

enum class BillingCycle {
    Weekly,
    Monthly,
    Quarterly,
    Yearly,
    CustomDays
};

struct Subscription {
    int id = -1;
    QString name;
    QString category;
    double amount = 0.0;
    QString currency = "BYN";
    BillingCycle cycle = BillingCycle::Monthly;
    int customDays = 30;
    QDate nextPaymentDate = QDate::currentDate();
    bool active = true;
    QString notes;

    double monthlyCost() const;
    double yearlyCost() const;
    int daysUntilPayment(const QDate& from = QDate::currentDate()) const;
    QDate calculateNextDateAfter(const QDate& date) const;

    static QString cycleToKey(BillingCycle cycle);
    static QString cycleToDisplayName(BillingCycle cycle);
    static BillingCycle keyToCycle(const QString& key);
};