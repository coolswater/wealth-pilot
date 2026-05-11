/**
 * @file pch.h
 * @brief 预编译头文件
 *
 * @details 包含常用的 Qt 和标准库头文件
 * 可以显著减少编译时间
 */

#ifndef PCH_H
#define PCH_H

// Qt Core
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QVector>
#include <QList>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QPair>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QSettings>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QReadWriteLock>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QScopedArrayPointer>
#include <QCoreApplication>

// Qt GUI
#include <QWidget>
#include <QMainWindow>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QTabWidget>
#include <QStackedWidget>
#include <QSplitter>
#include <QTreeView>
#include <QListView>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QAbstractItemModel>
#include <QItemDelegate>
#include <QStyledItemDelegate>

// Qt Layouts
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QSpacerItem>

// Qt Events
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QResizeEvent>

// Qt Network
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

// Qt SQL
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

// Standard Library
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <cstdint>

#endif // PCH_H
