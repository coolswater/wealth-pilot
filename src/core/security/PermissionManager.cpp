/**
 * @file PermissionManager.cpp
 * @brief 权限管理器实现
 */

#include "PermissionManager.h"
#include "utils/Logger.h"
#include <QUuid>
#include <QSettings>

const QString PermissionManager::ROLE_ADMIN = "admin";
const QString PermissionManager::ROLE_TRADER = "trader";
const QString PermissionManager::ROLE_VIEWER = "viewer";

PermissionManager* PermissionManager::instance()
{
    static PermissionManager* inst = new PermissionManager();
    return inst;
}

PermissionManager::PermissionManager(QObject* parent)
    : QObject(parent)
{
    initializeDefaultRoles();

    // 加载当前用户
    QSettings settings("WealthPilot", "Auth");
    m_currentUserId = settings.value("currentUser").toString();

    LOG_INFO("PermissionManager initialized");
}

void PermissionManager::initializeDefaultRoles()
{
    // 管理员角色
    UserRole admin;
    admin.id = ROLE_ADMIN;
    admin.name = "管理员";
    admin.description = "拥有所有权限";
    admin.permissions = {
        Permission::ViewMarket, Permission::ViewAccount, Permission::ViewPositions,
        Permission::ViewOrders, Permission::PlaceOrder, Permission::CancelOrder,
        Permission::ModifyOrder, Permission::ViewReports, Permission::ExportData,
        Permission::ManageAccounts, Permission::ManageUsers, Permission::ManageSettings,
        Permission::Admin
    };
    admin.createTime = QDateTime::currentDateTime();
    m_roles[admin.id] = admin;

    // 交易员角色
    UserRole trader;
    trader.id = ROLE_TRADER;
    trader.name = "交易员";
    trader.description = "可以查看和交易";
    trader.permissions = {
        Permission::ViewMarket, Permission::ViewAccount, Permission::ViewPositions,
        Permission::ViewOrders, Permission::PlaceOrder, Permission::CancelOrder,
        Permission::ModifyOrder, Permission::ViewReports
    };
    trader.createTime = QDateTime::currentDateTime();
    m_roles[trader.id] = trader;

    // 观察者角色
    UserRole viewer;
    viewer.id = ROLE_VIEWER;
    viewer.name = "观察者";
    viewer.description = "只能查看";
    viewer.permissions = {
        Permission::ViewMarket, Permission::ViewAccount, Permission::ViewPositions,
        Permission::ViewOrders, Permission::ViewReports
    };
    viewer.createTime = QDateTime::currentDateTime();
    m_roles[viewer.id] = viewer;

    LOG_DEBUG("Default roles initialized");
}

bool PermissionManager::addUser(const UserInfo& user)
{
    if (m_users.contains(user.id)) {
        LOG_WARNING(QString("User already exists: %1").arg(user.id));
        return false;
    }

    UserInfo newUser = user;
    if (newUser.id.isEmpty()) {
        newUser.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    newUser.createTime = QDateTime::currentDateTime();

    m_users[newUser.id] = newUser;

    emit userChanged(newUser.id);
    LOG_INFO(QString("User added: %1 (%2)").arg(newUser.username).arg(newUser.id));

    logAction("add_user", newUser.id, QString("Added user: %1").arg(newUser.username));

    return true;
}

bool PermissionManager::updateUser(const UserInfo& user)
{
    if (!m_users.contains(user.id)) {
        return false;
    }

    m_users[user.id] = user;

    emit userChanged(user.id);
    LOG_INFO(QString("User updated: %1").arg(user.id));

    logAction("update_user", user.id, QString("Updated user: %1").arg(user.username));

    return true;
}

bool PermissionManager::deleteUser(const QString& userId)
{
    if (!m_users.contains(userId)) {
        return false;
    }

    QString username = m_users[userId].username;
    m_users.remove(userId);

    emit userChanged(userId);
    LOG_INFO(QString("User deleted: %1").arg(userId));

    logAction("delete_user", userId, QString("Deleted user: %1").arg(username));

    return true;
}

UserInfo PermissionManager::getUser(const QString& userId) const
{
    return m_users.value(userId);
}

QVector<UserInfo> PermissionManager::getAllUsers() const
{
    return m_users.values();
}

void PermissionManager::setCurrentUser(const QString& userId)
{
    m_currentUserId = userId;

    QSettings settings("WealthPilot", "Auth");
    settings.setValue("currentUser", userId);

    if (m_users.contains(userId)) {
        m_users[userId].lastLogin = QDateTime::currentDateTime();
    }

    LOG_INFO(QString("Current user set: %1").arg(userId));
}

UserInfo PermissionManager::currentUser() const
{
    return m_users.value(m_currentUserId);
}

bool PermissionManager::addRole(const UserRole& role)
{
    if (m_roles.contains(role.id)) {
        return false;
    }

    UserRole newRole = role;
    newRole.createTime = QDateTime::currentDateTime();
    m_roles[newRole.id] = newRole;

    emit roleChanged(newRole.id);
    LOG_INFO(QString("Role added: %1").arg(newRole.name));

    logAction("add_role", newRole.id, QString("Added role: %1").arg(newRole.name));

    return true;
}

bool PermissionManager::updateRole(const UserRole& role)
{
    if (!m_roles.contains(role.id)) {
        return false;
    }

    m_roles[role.id] = role;

    emit roleChanged(role.id);
    LOG_INFO(QString("Role updated: %1").arg(role.id));

    logAction("update_role", role.id, QString("Updated role: %1").arg(role.name));

    return true;
}

bool PermissionManager::deleteRole(const QString& roleId)
{
    if (!m_roles.contains(roleId)) {
        return false;
    }

    // 不允许删除默认角色
    if (roleId == ROLE_ADMIN || roleId == ROLE_TRADER || roleId == ROLE_VIEWER) {
        LOG_WARNING(QString("Cannot delete default role: %1").arg(roleId));
        return false;
    }

    QString name = m_roles[roleId].name;
    m_roles.remove(roleId);

    emit roleChanged(roleId);
    LOG_INFO(QString("Role deleted: %1").arg(roleId));

    logAction("delete_role", roleId, QString("Deleted role: %1").arg(name));

    return true;
}

UserRole PermissionManager::getRole(const QString& roleId) const
{
    return m_roles.value(roleId);
}

QVector<UserRole> PermissionManager::getAllRoles() const
{
    return m_roles.values();
}

bool PermissionManager::hasPermission(const QString& userId, Permission permission) const
{
    if (!m_users.contains(userId)) {
        return false;
    }

    QString roleId = m_users[userId].roleId;
    if (!m_roles.contains(roleId)) {
        return false;
    }

    bool has = m_roles[roleId].permissions.contains(permission);

    if (!has) {
        LOG_DEBUG(QString("Permission denied: user=%1, permission=%2")
            .arg(userId).arg(static_cast<int>(permission)));
    }

    return has;
}

bool PermissionManager::hasPermission(Permission permission) const
{
    return hasPermission(m_currentUserId, permission);
}

QSet<Permission> PermissionManager::getUserPermissions(const QString& userId) const
{
    if (!m_users.contains(userId)) {
        return {};
    }

    QString roleId = m_users[userId].roleId;
    return m_roles.value(roleId).permissions;
}

bool PermissionManager::grantPermission(const QString& roleId, Permission permission)
{
    if (!m_roles.contains(roleId)) {
        return false;
    }

    m_roles[roleId].permissions.insert(permission);

    emit roleChanged(roleId);
    LOG_INFO(QString("Permission granted: role=%1, permission=%2")
        .arg(roleId).arg(static_cast<int>(permission)));

    logAction("grant_permission", roleId, QString("Granted permission: %1")
        .arg(static_cast<int>(permission)));

    return true;
}

bool PermissionManager::revokePermission(const QString& roleId, Permission permission)
{
    if (!m_roles.contains(roleId)) {
        return false;
    }

    m_roles[roleId].permissions.remove(permission);

    emit roleChanged(roleId);
    LOG_INFO(QString("Permission revoked: role=%1, permission=%2")
        .arg(roleId).arg(static_cast<int>(permission)));

    logAction("revoke_permission", roleId, QString("Revoked permission: %1")
        .arg(static_cast<int>(permission)));

    return true;
}

void PermissionManager::logAction(const QString& action, const QString& resource,
                                  const QString& detail, bool success)
{
    AuditLog log;
    log.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    log.userId = m_currentUserId;
    log.action = action;
    log.resource = resource;
    log.detail = detail;
    log.time = QDateTime::currentDateTime();
    log.success = success;

    m_auditLogs.append(log);

    LOG_DEBUG(QString("Audit log: %1 - %2").arg(action).arg(resource));
}

QVector<AuditLog> PermissionManager::getAuditLogs(const QString& userId,
                                                   const QDateTime& from,
                                                   const QDateTime& to) const
{
    QVector<AuditLog> result;

    for (const AuditLog& log : m_auditLogs) {
        if (!userId.isEmpty() && log.userId != userId) {
            continue;
        }
        if (from.isValid() && log.time < from) {
            continue;
        }
        if (to.isValid() && log.time > to) {
            continue;
        }
        result.append(log);
    }

    return result;
}

void PermissionManager::clearAuditLogs(const QDateTime& before)
{
    if (!before.isValid()) {
        m_auditLogs.clear();
    } else {
        for (int i = m_auditLogs.size() - 1; i >= 0; --i) {
            if (m_auditLogs[i].time < before) {
                m_auditLogs.removeAt(i);
            }
        }
    }

    LOG_INFO(QString("Audit logs cleared before: %1").arg(before.toString()));
}