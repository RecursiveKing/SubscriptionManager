#pragma once

#include <QMainWindow>
#include "database/DatabaseManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private:
    DatabaseManager m_db;
};