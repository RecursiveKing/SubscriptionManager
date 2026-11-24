cmake_minimum_required(VERSION 3.16)
project(SubscriptionManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Charts)

add_executable(SubscriptionManager
    src/main.cpp
    src/models/Subscription.h
    src/models/Subscription.cpp
    src/database/DatabaseManager.h
    src/database/DatabaseManager.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/SubscriptionDialog.h
    src/ui/SubscriptionDialog.cpp
    src/ui/AnalyticsWidget.h
    src/ui/AnalyticsWidget.cpp
)

target_include_directories(SubscriptionManager PRIVATE src)

target_link_libraries(SubscriptionManager PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::Charts
)