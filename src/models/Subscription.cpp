#include "Subscription.h"

double Subscription::monthlyCost() const {
    if (!active) {
        return 0.0;
    }

    switch (cycle) {
        case BillingCycle::Weekly:
            return amount * 52.0 / 12.0;
        case BillingCycle::Monthly:
            return amount;
        case BillingCycle::Quarterly:
            return amount / 3.0;
        case BillingCycle::Yearly:
            return amount / 12.0;
        case BillingCycle::CustomDays:
            return customDays > 0 ? amount * (30.4375 / customDays) : 0.0;
    }
    return 0.0;
}

double Subscription::yearlyCost() const {
    if (!active) {
        return 0.0;
    }

    switch (cycle) {
        case BillingCycle::Weekly:
            return amount * 52.0;
        case BillingCycle::Monthly:
            return amount * 12.0;
        case BillingCycle::Quarterly:
            return amount * 4.0;
        case BillingCycle::Yearly:
            return amount;
        case BillingCycle::CustomDays:
            return customDays > 0 ? amount * (365.25 / customDays) : 0.0;
    }
    return 0.0;
}

int Subscription::daysUntilPayment(const QDate& from) const {
    return static_cast<int>(from.daysTo(nextPaymentDate));
}

QDate Subscription::calculateNextDateAfter(const QDate& date) const {
    switch (cycle) {
        case BillingCycle::Weekly:
            return date.addDays(7);
        case BillingCycle::Monthly:
            return date.addMonths(1);
        case BillingCycle::Quarterly:
            return date.addMonths(3);
        case BillingCycle::Yearly:
            return date.addYears(1);
        case BillingCycle::CustomDays:
            return date.addDays(customDays > 0 ? customDays : 30);
    }
    return date.addMonths(1);
}

QString Subscription::cycleToKey(BillingCycle cycle) {
    switch (cycle) {
        case BillingCycle::Weekly:     return "weekly";
        case BillingCycle::Monthly:    return "monthly";
        case BillingCycle::Quarterly:  return "quarterly";
        case BillingCycle::Yearly:     return "yearly";
        case BillingCycle::CustomDays: return "custom";
    }
    return "monthly";
}

QString Subscription::cycleToDisplayName(BillingCycle cycle) {
    switch (cycle) {
        case BillingCycle::Weekly:     return "Еженедельно";
        case BillingCycle::Monthly:    return "Ежемесячно";
        case BillingCycle::Quarterly:  return "Ежеквартально";
        case BillingCycle::Yearly:     return "Ежегодно";
        case BillingCycle::CustomDays: return "Свой интервал";
    }
    return "Ежемесячно";
}

BillingCycle Subscription::keyToCycle(const QString& key) {
    if (key == "weekly")    return BillingCycle::Weekly;
    if (key == "quarterly") return BillingCycle::Quarterly;
    if (key == "yearly")    return BillingCycle::Yearly;
    if (key == "custom")    return BillingCycle::CustomDays;
    return BillingCycle::Monthly;
}