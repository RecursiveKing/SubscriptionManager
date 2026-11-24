#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "models/Subscription.h"

struct PaymentRecord {
    int id = -1;
    int subscriptionId = -1;
    QString subscriptionName;
    QDate paidDate;
    double amount = 0.0;
    QString currency = "BYN";
};

class DatabaseManager {
public:
    DatabaseManager() = default;
    ~DatabaseManager();

    bool init(const QString& dbPath);

    QVector<Subscription> getAllSubscriptions() const;
    bool addSubscription(Subscription& sub);
    bool updateSubscription(const Subscription& sub);
    bool deleteSubscription(int id);

    bool markAsPaid(Subscription& sub);
    QVector<PaymentRecord> getPaymentHistory() const;

    QString lastError() const { return m_lastError; }

private:
    bool createTables();

    QSqlDatabase m_db;
    QString m_lastError;
};