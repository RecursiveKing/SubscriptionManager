#include "DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::init(const QString& dbPath) {
    if (QSqlDatabase::contains("main_connection")) {
        m_db = QSqlDatabase::database("main_connection");
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "main_connection");
    }

    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(m_db);
    pragmaQuery.exec("PRAGMA foreign_keys = ON;");

    return createTables();
}

bool DatabaseManager::createTables() {
    QSqlQuery query(m_db);

    const QString createSubscriptionsSql = R"(
        CREATE TABLE IF NOT EXISTS subscriptions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            category TEXT NOT NULL,
            amount REAL NOT NULL,
            currency TEXT NOT NULL DEFAULT 'BYN',
            billing_cycle TEXT NOT NULL,
            custom_days INTEGER NOT NULL DEFAULT 30,
            next_payment_date TEXT NOT NULL,
            is_active INTEGER NOT NULL DEFAULT 1,
            notes TEXT DEFAULT ''
        );
    )";

    if (!query.exec(createSubscriptionsSql)) {
        m_lastError = query.lastError().text();
        return false;
    }

    const QString createHistorySql = R"(
        CREATE TABLE IF NOT EXISTS payment_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            subscription_id INTEGER NOT NULL,
            paid_date TEXT NOT NULL,
            amount REAL NOT NULL,
            currency TEXT NOT NULL,
            FOREIGN KEY (subscription_id) REFERENCES subscriptions(id) ON DELETE CASCADE
        );
    )";

    if (!query.exec(createHistorySql)) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QVector<Subscription> DatabaseManager::getAllSubscriptions() const {
    QVector<Subscription> list;
    QSqlQuery query(m_db);

    const QString sql = R"(
        SELECT id, name, category, amount, currency,
               billing_cycle, custom_days, next_payment_date, is_active, notes
        FROM subscriptions
        ORDER BY is_active DESC, next_payment_date ASC;
    )";

    if (!query.exec(sql)) {
        return list;
    }

    while (query.next()) {
        Subscription sub;
        sub.id = query.value(0).toInt();
        sub.name = query.value(1).toString();
        sub.category = query.value(2).toString();
        sub.amount = query.value(3).toDouble();
        sub.currency = query.value(4).toString();
        sub.cycle = Subscription::keyToCycle(query.value(5).toString());
        sub.customDays = query.value(6).toInt();
        sub.nextPaymentDate = QDate::fromString(query.value(7).toString(), Qt::ISODate);
        sub.active = query.value(8).toInt() != 0;
        sub.notes = query.value(9).toString();
        list.push_back(sub);
    }

    return list;
}

bool DatabaseManager::addSubscription(Subscription& sub) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO subscriptions
            (name, category, amount, currency, billing_cycle, custom_days, next_payment_date, is_active, notes)
        VALUES
            (:name, :category, :amount, :currency, :cycle, :custom_days, :next_date, :active, :notes);
    )");

    query.bindValue(":name", sub.name);
    query.bindValue(":category", sub.category);
    query.bindValue(":amount", sub.amount);
    query.bindValue(":currency", sub.currency);
    query.bindValue(":cycle", Subscription::cycleToKey(sub.cycle));
    query.bindValue(":custom_days", sub.customDays);
    query.bindValue(":next_date", sub.nextPaymentDate.toString(Qt::ISODate));
    query.bindValue(":active", sub.active ? 1 : 0);
    query.bindValue(":notes", sub.notes);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    sub.id = query.lastInsertId().toInt();
    return true;
}

bool DatabaseManager::updateSubscription(const Subscription& sub) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE subscriptions
        SET name = :name,
            category = :category,
            amount = :amount,
            currency = :currency,
            billing_cycle = :cycle,
            custom_days = :custom_days,
            next_payment_date = :next_date,
            is_active = :active,
            notes = :notes
        WHERE id = :id;
    )");

    query.bindValue(":name", sub.name);
    query.bindValue(":category", sub.category);
    query.bindValue(":amount", sub.amount);
    query.bindValue(":currency", sub.currency);
    query.bindValue(":cycle", Subscription::cycleToKey(sub.cycle));
    query.bindValue(":custom_days", sub.customDays);
    query.bindValue(":next_date", sub.nextPaymentDate.toString(Qt::ISODate));
    query.bindValue(":active", sub.active ? 1 : 0);
    query.bindValue(":notes", sub.notes);
    query.bindValue(":id", sub.id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteSubscription(int id) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM subscriptions WHERE id = :id;");
    query.bindValue(":id", id);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::markAsPaid(Subscription& sub) {
    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        return false;
    }

    QSqlQuery insertHistory(m_db);
    insertHistory.prepare(R"(
        INSERT INTO payment_history (subscription_id, paid_date, amount, currency)
        VALUES (:sub_id, :paid_date, :amount, :currency);
    )");
    insertHistory.bindValue(":sub_id", sub.id);
    insertHistory.bindValue(":paid_date", QDate::currentDate().toString(Qt::ISODate));
    insertHistory.bindValue(":amount", sub.amount);
    insertHistory.bindValue(":currency", sub.currency);

    if (!insertHistory.exec()) {
        m_lastError = insertHistory.lastError().text();
        m_db.rollback();
        return false;
    }

    QDate newNextDate = sub.calculateNextDateAfter(sub.nextPaymentDate);

    QSqlQuery updateSub(m_db);
    updateSub.prepare("UPDATE subscriptions SET next_payment_date = :next_date WHERE id = :id;");
    updateSub.bindValue(":next_date", newNextDate.toString(Qt::ISODate));
    updateSub.bindValue(":id", sub.id);

    if (!updateSub.exec()) {
        m_lastError = updateSub.lastError().text();
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    sub.nextPaymentDate = newNextDate;
    return true;
}

QVector<PaymentRecord> DatabaseManager::getPaymentHistory() const {
    QVector<PaymentRecord> records;
    QSqlQuery query(m_db);

    const QString sql = R"(
        SELECT h.id, h.subscription_id, s.name, h.paid_date, h.amount, h.currency
        FROM payment_history h
        JOIN subscriptions s ON s.id = h.subscription_id
        ORDER BY h.paid_date DESC, h.id DESC;
    )";

    if (!query.exec(sql)) {
        return records;
    }

    while (query.next()) {
        PaymentRecord rec;
        rec.id = query.value(0).toInt();
        rec.subscriptionId = query.value(1).toInt();
        rec.subscriptionName = query.value(2).toString();
        rec.paidDate = QDate::fromString(query.value(3).toString(), Qt::ISODate);
        rec.amount = query.value(4).toDouble();
        rec.currency = query.value(5).toString();
        records.push_back(rec);
    }

    return records;
}