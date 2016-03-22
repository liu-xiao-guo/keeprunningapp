#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include <QAbstractListModel>
#include <QDir>

class QGSettings;


class ApplicationItem: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(QString appId READ appId CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(bool lifecycleException READ lifecycleException WRITE setLifecycleException NOTIFY lifecycleExceptionChanged)
    Q_PROPERTY(int hooksCount READ hooksCount CONSTANT)
    Q_PROPERTY(quint64 appSize READ appSize NOTIFY diskSpaceChanged)
    Q_PROPERTY(quint64 cacheSize READ cacheSize NOTIFY diskSpaceChanged)
    Q_PROPERTY(quint64 dataSize READ dataSize NOTIFY diskSpaceChanged)
    Q_PROPERTY(quint64 configSize READ configSize NOTIFY diskSpaceChanged)

    Q_ENUMS(Hook)
    Q_FLAGS(Hooks)
public:
    enum Hook {
        HookNone = 0,
        HookDesktop = 1,
        HookScope = 2,
        HookAccountService = 4,
        HookUrls = 8,
        HookContentHub = 16,
        HookPushHelper = 32
    };
    Q_DECLARE_FLAGS(Hooks, Hook)

    struct HookStruct {
        QString name;
        Hooks hooks;
        QStringList permissions;
        QString apparmorTemplate;
        QStringList readPaths;
        QStringList writePaths;
    };

    explicit ApplicationItem(const QString &appId, const QString &version, const QString &name, const QString &icon, const QString &description, QObject *parent = 0):
        QObject(parent),
        m_appId(appId),
        m_version(version),
        m_name(name),
        m_icon(icon),
        m_description(description),
        m_lifecycleException(false),
        m_appSize(0),
        m_cacheSize(0),
        m_dataSize(0),
        m_configSize(0)
    {}

    QString name() const { return m_name; }
    QString icon() const { return m_icon; }
    QString appId() const { return m_appId; }
    QString version() const { return m_version; }
    QString description() const { return m_description; }
    Q_INVOKABLE QString permissions(int index) const { return m_hooks.at(index).permissions.join(", "); }
    Q_INVOKABLE Hooks hooks(int index) const { return m_hooks.at(index).hooks; }
    Q_INVOKABLE QString hookName(int index) { return m_hooks.at(index).name; }
    Q_INVOKABLE QString apparmorTemplate(int index) { return m_hooks.at(index).apparmorTemplate; }
    Q_INVOKABLE QString readPaths(int index) { return m_hooks.at(index).readPaths.join(", "); }
    Q_INVOKABLE QString writePaths(int index) { return m_hooks.at(index).writePaths.join(", "); }
    int hooksCount() const { return m_hooks.count(); }

    void setHooks(QList<HookStruct> hooksStruct) { m_hooks = hooksStruct; }

    bool lifecycleException() const { return m_lifecycleException; }
    void setLifecycleException(bool exception) {
        if (m_lifecycleException != exception) {
            m_lifecycleException = exception;
            Q_EMIT lifecycleExceptionChanged();
        }
    }

    int appSize() const { return m_appSize; }
    int cacheSize() const { return m_cacheSize; }
    int dataSize() const { return m_dataSize; }
    int configSize() const { return m_configSize; }

public Q_SLOTS:
    void calculateSize();

Q_SIGNALS:
    void lifecycleExceptionChanged();
    void diskSpaceChanged();

private:
    quint64 dirSize(const QString &dir) const;

private:
    QString m_appId;
    QString m_version;
    QString m_name;
    QString m_icon;
    QString m_description;

    QList<HookStruct> m_hooks;
    bool m_lifecycleException;

    quint64 m_appSize;
    quint64 m_cacheSize;
    quint64 m_dataSize;
    quint64 m_configSize;
};

class ApplicationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        RoleName,
        RoleVersion,
        RoleIcon,
        RoleDescription,
        RoleLifecycleException
    };

    explicit ApplicationModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE ApplicationItem* get(int index) const;
    Q_INVOKABLE int count() { return m_appList.count(); }

private Q_SLOTS:
    void appLifecycleExceptionChanged();

private:
    void buildClickList();
    QString getIconFromDesktopFile(const QString &desktopFile);
    QString getIconFromScopeFile(const QString &scopeFile);

    QList<ApplicationItem*> m_appList;

    QGSettings *m_settings;
};

#endif // APPLICATIONMODEL_H
