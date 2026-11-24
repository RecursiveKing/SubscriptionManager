#include "MainWindow.h"

#include <QCoreApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Менеджер подписок и регулярных платежей");
    resize(960, 620);

    const QString dbPath = QCoreApplication::applicationDirPath() + "/subscriptions.db";
    bool ok = m_db.init(dbPath);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    QString statusText = ok
        ? QString("База данных подключена!\nФайл: %1\nПодписок в базе: %2")
              .arg(dbPath)
              .arg(m_db.getAllSubscriptions().size())
        : QString("Ошибка БД: %1").arg(m_db.lastError());

    auto* label = new QLabel(statusText, central);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font-size: 16px;");

    layout->addWidget(label);
    setCentralWidget(central);
}