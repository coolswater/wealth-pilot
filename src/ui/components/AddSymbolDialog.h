/**
* @file AddSymbolDialog.h
 * @brief 添加自选对话框
 * @details 支持搜索交易对、分类显示、快速选择
 */

#ifndef ADDSYMBOLDIALOG_H
#define ADDSYMBOLDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>

/**
 * @class AddSymbolDialog
 * @brief 添加自选交易对对话框
 */
class AddSymbolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddSymbolDialog(const QStringList &availableSymbols,
                            const QStringList &existingSymbols,
                            QWidget *parent = nullptr);

    /**
     * @brief 获取用户选中的交易对
     */
    QStringList selectedSymbols() const;

private slots:
    void onSearchTextChanged(const QString &text) const;
    void onSymbolSelected(const QListWidgetItem *item);
    void onCategoryChanged(int index);
    void updateSearchResults();

private:
    void setupUI();
    void filterSymbols(const QString &filter);
    void loadCategories();

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_symbolList = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    QStringList m_allSymbols;       ///< 所有可用交易对
    QStringList m_existingSymbols;  ///< 已添加的自选
    QStringList m_selectedSymbols;  ///< 当前选中的
    QTimer *m_searchTimer = nullptr; ///< 搜索防抖定时器
};

#endif // ADDSYMBOLDIALOG_H