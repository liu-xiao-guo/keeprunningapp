#define QT_NO_KEYWORDS

#include "applicationmodel.h"


#include <click.h>
#include <gio/gio.h>
//#include <gio/gdesktopappinfo.h>
#include <glib.h>
#include <libintl.h>

#include <QDebug>
#include <QJsonParseError>
#include <QGSettings>
#include <QSettings>
#include <QFile>
#include <QFileInfo>

ApplicationModel::ApplicationModel(QObject *parent) :
    QAbstractListModel(parent)
{
    m_settings = new QGSettings("com.canonical.qtmir", "/com/canonical/qtmir/", this);
    buildClickList();

}

int ApplicationModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_appList.count();
}

QVariant ApplicationModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case RoleName:
        return m_appList.at(index.row())->name();
    case RoleVersion:
        return m_appList.at(index.row())->version();
    case RoleIcon:
        return m_appList.at(index.row())->icon();
    case RoleDescription:
        return m_appList.at(index.row())->description();
    case RoleLifecycleException:
        return m_appList.at(index.row())->lifecycleException();
    }

    return QVariant();
}

QHash<int, QByteArray> ApplicationModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(RoleName, "name");
    roles.insert(RoleVersion, "version");
    roles.insert(RoleIcon, "icon");
    roles.insert(RoleDescription, "description");
    roles.insert(RoleLifecycleException, "lifecycleException");
    return roles;
}

ApplicationItem *ApplicationModel::get(int index) const
{
    return m_appList.at(index);
}

void ApplicationModel::appLifecycleExceptionChanged()
{
    ApplicationItem *item = qobject_cast<ApplicationItem*>(sender());

    QStringList exceptionList = m_settings->get("lifecycleExemptAppids").toStringList();
    if (item->lifecycleException() && !exceptionList.contains(item->appId())) {
        exceptionList.append(item->appId());
    } else if (!item->lifecycleException() && exceptionList.contains(item->appId())) {
        exceptionList.removeAll(item->appId());
    }
    m_settings->set("lifecycleExemptAppids", exceptionList);

    int idx = m_appList.indexOf(item);
    Q_EMIT dataChanged(index(idx), index(idx), QVector<int>() << RoleLifecycleException);
}

void ApplicationModel::buildClickList()
{
    ClickDB *clickdb;
     GError *err = nullptr;
     gchar *clickmanifest = nullptr;

     clickdb = click_db_new();
     click_db_read(clickdb, nullptr, &err);
     if (err != nullptr) {
         g_warning("Unable to read Click database: %s", err->message);
         g_error_free(err);
         g_object_unref(clickdb);
         return;
     }

     clickmanifest = click_db_get_manifests_as_string(clickdb, TRUE, &err);
     g_object_unref(clickdb);

     if (err != nullptr) {
         g_warning("Unable to get the manifests: %s", err->message);
         g_error_free(err);
         return;
     }

     QJsonParseError error;

     QJsonDocument jsond =
             QJsonDocument::fromJson(clickmanifest, &error);
     g_free(clickmanifest);

     if (error.error != QJsonParseError::NoError) {
         qWarning() << error.errorString();
         return;
     }

     qDebug() << "loaded stuff" << jsond.toJson();
     QVariantList appListJson = jsond.toVariant().toList();

     QStringList exceptionList = m_settings->get("lifecycleExemptAppids").toStringList();
     qDebug() << "exceptions" << exceptionList;

     Q_FOREACH(const QVariant &appJson, appListJson) {
         QVariantMap appMap = appJson.toMap();

         QString appId = appMap.value("name").toString();
         QString appName = appMap.value("title").toString();
         QString appDescription = appMap.value("description").toString();
         QString version = appMap.value("version").toString();

         QString appIcon;
         QString path = appMap.value("_directory").toString();
         QList<ApplicationItem::HookStruct> hooksList;
         if (appMap.contains("icon")) {
             appIcon = "file://" + path + "/" + appMap.value("icon").toString();
         }
         if(appMap.contains("hooks")) {
             QVariantMap hookMap = appMap.value("hooks").toMap();
             Q_FOREACH (const QString & hook, hookMap.keys()) {
                 QFile apparmorFile("/var/lib/apparmor/clicks/" + appId + "_" + hook + "_" + version + ".json");
                 ApplicationItem::HookStruct hookStruct;
                 hookStruct.name = hook;
                 hookStruct.hooks = ApplicationItem::HookNone;
                 qDebug() << "opening apparmor file" << apparmorFile.fileName();
                 QStringList permissions;
                 QStringList readPaths;
                 QStringList writePaths;
                 if (apparmorFile.open(QFile::ReadOnly)) {
                     qDebug() << "opened apparmor file" << apparmorFile.fileName();
                     QJsonParseError pe;
                     QJsonDocument jsonDoc = QJsonDocument::fromJson(apparmorFile.readAll(), &pe);
                     if (pe.error == QJsonParseError::NoError) {
                         QVariantMap apparmor = jsonDoc.toVariant().toMap();
                         qDebug() << "have json:" << apparmor;
                         Q_FOREACH (const QVariant &perm, apparmor.value("policy_groups").toList()) {
                             permissions.append(perm.toString());
                         }
                         Q_FOREACH (const QVariant &perm, apparmor.value("read_path").toList()) {
                             readPaths.append(perm.toString());
                         }
                         Q_FOREACH (const QVariant &perm, apparmor.value("write_paths").toList()) {
                             writePaths.append(perm.toString());
                         }
                         hookStruct.apparmorTemplate = apparmor.value("template").toString();
                     }
                 }
                 hookStruct.readPaths = readPaths;
                 hookStruct.writePaths = writePaths;
                 hookStruct.permissions = permissions;

                 if (hookMap.value(hook).toMap().contains("desktop")) {
                     QString desktopFile = hookMap.value(hook).toMap().value("desktop").toString();
                     QString iconFileName = getIconFromDesktopFile(path + "/" + desktopFile);
                     appIcon = "file://" + path + "/" + iconFileName;
                     hookStruct.hooks |= ApplicationItem::HookDesktop;
                 }
                 if (hookMap.value(hook).toMap().contains("scope")) {
                     hookStruct.hooks |= ApplicationItem::HookScope;
                     QString scopedir = hookMap.value(hook).toMap().value("scope").toString();
                     QString inifile = appId + "_" + scopedir + ".ini";
                     QString iconFileName = getIconFromScopeFile(path + "/" + scopedir + "/" + inifile);
                     appIcon = "file://" + path + "/" + scopedir + "/" + iconFileName;
                 }
                 if (hookMap.value(hook).toMap().contains("content-hub")) {
                     hookStruct.hooks |= ApplicationItem::HookContentHub;
                 }
                 if (hookMap.value(hook).toMap().contains("urls")) {
                     hookStruct.hooks |= ApplicationItem::HookUrls;
                 }
                 if (hookMap.value(hook).toMap().contains("push-helper")) {
                     hookStruct.hooks |= ApplicationItem::HookPushHelper;
                 }
                 if (hookMap.value(hook).toMap().contains("account-provider")) {
                     hookStruct.hooks |= ApplicationItem::HookAccountService;
                 }
                 hooksList.append(hookStruct);
             }
         }


         ApplicationItem *item = new ApplicationItem(appId, version, appName, appIcon, appDescription, this);
         item->setHooks(hooksList);
         item->setLifecycleException(exceptionList.contains(item->appId()));
         connect(item, &ApplicationItem::lifecycleExceptionChanged, this, &ApplicationModel::appLifecycleExceptionChanged);
         m_appList.append(item);
     }
}

QString ApplicationModel::getIconFromDesktopFile(const QString &desktopFile)
{
    QSettings settings(desktopFile, QSettings::IniFormat);
    qDebug() << "loading desktopfile" << settings.allKeys();
    return settings.value("Desktop Entry/Icon").toString();
}

QString ApplicationModel::getIconFromScopeFile(const QString &scopeFile)
{
    QSettings settings(scopeFile, QSettings::IniFormat);
    qDebug() << "loading scopeFile" << settings.allKeys();
    return settings.value("ScopeConfig/Icon").toString();
}


void ApplicationItem::calculateSize()
{
    m_appSize = dirSize("/opt/click.ubuntu.com/" + m_appId + "/" + m_version + "/");
    m_cacheSize = dirSize("/home/phablet/.cache/" + m_appId);
    m_configSize = dirSize("/home/phablet/.config/" + m_appId);
    m_dataSize = dirSize("/home/phablet/.local/share/" + m_appId);
    Q_EMIT diskSpaceChanged();
}

quint64 ApplicationItem::dirSize(const QString &dir) const
{
    qDebug() << "checking size of" << dir;
    quint64 size = 0;
    Q_FOREACH (const QString &path, QDir(dir).entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        qDebug() << "fi:" << path;
        QFileInfo fi(dir + "/" + path);
        if (fi.isDir()) {
            size += dirSize(dir + "/" + path);
        } else if (fi.isFile()) {
            size += fi.size();
        }
    }
    return size;
}
