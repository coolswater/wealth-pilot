/**
 * @file PermissionManager.h
 * @brief 权限管理器 - 企业级权限控制
 *
 * @details 功能：
 * - 角色管理
 * - 权限控制
 * - 操作审计
 * - 访问控制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <QObject>
#include <QSet>
#include <QHash>
#include <QString>
#include <QDateTime>

/**
 * @brief 权限类型
 */
enum class Permission {
    ViewMarket,         ///< 查看行情
    ViewAccount,        ///< 查看账户
    ViewPositions,      ///< 查看持仓
    ViewOrders,         ///< 查看订单
    PlaceOrder,         ///< 下单
    CancelOrder,        ///< 撤单
    ModifyOrder,        ///< 改单
    ViewReports,        ///< 查看报告
    ExportData,         ///< 导出数据
    ManageAccounts,     ///< 管理账户
    ManageUsers,        ///< 管理用户
    ManageSettings,     ///< 管理设置
    Admin               ///< 管理员权限
};

/**
 * @brief 用户角色
 */
struct UserRole {
    QString id;                 ///< 角色ID
    QString name;               ///< 角色名称
    QString description;        ///< 描述
    QSet<Permission> permissions; ///< 权限集合
    QDateTime createTime;       ///< 创建时间
};

/**
 * @brief 用户信息
 */
struct UserInfo {
    QString id;                 ///< 用户ID
    QString username;           ///< 用户名
    QString displayName;        ///< 显示名称
    QString email;              ///< 邮箱
    QString roleId;             ///< 角色ID
    bool enabled = true;        ///< 是否启用
    QDateTime lastLogin;        ///< 最后登录时间
    QDateTime createTime;       ///< 创建时间
};

/**
 * @brief 审计日志
 */
struct AuditLog {
    QString id;                 ///< 日志ID
    QString userId;             ///< 用户ID
    QString action;             ///< 动作
    QString resource;           ///< 资源
    QString detail;             ///< 详情
    QString ipAddress;          ///< IP地址
    QDateTime time;             ///< 时间
    bool success = true;        ///< 是否成功
};

/**
 * @brief 权限管理器
 *
 * 提供企业级权限管理：
 * - 角色管理
 * - 权限控制
 * - 操作审计
 */
class PermissionManager : public QObject {
    Q_OBJECT

public:
    static PermissionManager* instance();

    // ========== 用户管理 ==========

    /**
     * @brief 添加用户
     */
    bool addUser(const UserInfo& user);

    /**
     * @brief 更新用户
     */
    bool updateUser(const UserInfo& user);

    /**
     * @brief 删除用户
     */
    bool deleteUser(const QString& userId);

    /**
     * @brief 获取用户
     */
    UserInfo getUser(const QString& userId) const;

    /**
     * @brief 获取所有用户
     */
    QVector<UserInfo> getAllUsers() const;

    /**
     * @brief 设置当前用户
     */
    void setCurrentUser(const QString& userId);

    /**
     * @brief 获取当前用户
     */
    UserInfo currentUser() const;

    // ========== 角色管理 ==========

    /**
     * @brief 添加角色
     */
    bool addRole(const UserRole& role);

    /**
     * @brief 更新角色
     */
    bool updateRole(const UserRole& role);

    /**
     * @brief 删除角色
     */
    bool deleteRole(const QString& roleId);

    /**
     * @brief 获取角色
     */
    UserRole getRole(const QString& roleId) const;

    /**
     * @brief 获取所有角色
     */
    QVector<UserRole> getAllRoles() const;

    // ========== 权限检查 ==========

    /**
     * @brief 检查权限
     */
    bool hasPermission(const QString& userId, Permission permission) const;

    /**
     * @brief 检查当前用户权限
     */
    bool hasPermission(Permission permission) const;

    /**
     * @brief 获取用户权限列表
     */
    QSet<Permission> getUserPermissions(const QString& userId) const;

    /**
     * @brief 授予权限
     */
    bool grantPermission(const QString& roleId, Permission permission);

    /**
     * @brief 撤销权限
     */
    bool revokePermission(const QString& roleId, Permission permission);

    // ========== 审计日志 ==========

    /**
     * @brief 记录操作
     */
    void logAction(const QString& action, const QString& resource,
                  const QString& detail, bool success = true);

    /**
     * @brief 获取审计日志
     */
    QVector<AuditLog> getAuditLogs(const QString& userId = QString(),
                                   const QDateTime& from = QDateTime(),
                                   const QDateTime& to = QDateTime()) const;

    /**
     * @brief 清除审计日志
     */
    void clearAuditLogs(const QDateTime& before);

signals:
    /**
     * @brief 用户变化信号
     */
    void userChanged(const QString& userId);

    /**
     * @brief 角色变化信号
     */
    void roleChanged(const QString& roleId);

    /**
     * @brief 权限检查失败信号
     */
    void permissionDenied(const QString& userId, Permission permission);

private:
    explicit PermissionManager(QObject* parent = nullptr);
    ~PermissionManager() override = default;

    void initializeDefaultRoles();

    QHash<QString, UserInfo> m_users;
    QHash<QString, UserRole> m_roles;
    QVector<AuditLog> m_auditLogs;
    QString m_currentUserId;

    // 默认角色
    static const QString ROLE_ADMIN;
    static const QString ROLE_TRADER;
    static const QString ROLE_VIEWER;
};

#endif // PERMISSIONMANAGER_H