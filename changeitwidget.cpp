#include <QMessageBox>
#include <QDateTime>
#include <QInputDialog>
#include <QDebug>
#include <QAbstractListModel>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QSettings>
#include <QFileDialog>
#include <QTimer>
#include "changeitwidget.h"
#include "ui_changeitwidget.h"

QString makeProfilePath()
{
    return QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
}


class Profile{
public:
    Profile(){}

    Profile(const QDir& profileDir)
    {
        this->profileDir = profileDir;
        QString infoFile = this->profileDir.filePath("info.ini");
        QSettings settings(infoFile, QSettings::IniFormat);
        settings.setIniCodec("gbk");
        this->name = settings.value("name").toString();
    }

    void setName(const QString& newName)
    {
        this->name = newName;
        QString infoFile = this->profileDir.filePath("info.ini");
        QSettings settings(infoFile, QSettings::IniFormat);
        settings.setIniCodec("gbk");
        settings.setValue("name", newName);
    }

    QString getName() const
    {
        return this->name;
    }

    void deleteFromDisk()
    {
        this->profileDir.removeRecursively();
    }

    bool isValid()
    {
        return !this->name.isEmpty();
    }

    const QDir& getProfileDir()
    {
        return this->profileDir;
    }

private:
    QString name;
    QDir profileDir;
};


class ProfileModel: public QAbstractListModel{
public:
    virtual int rowCount(const QModelIndex &parent) const
    {
        if(parent.isValid())
            return 0;
        return this->profiles.size();
    }

    virtual QVariant data(const QModelIndex &index, int role) const
    {
        if(role == Qt::DisplayRole && index.column() == 0) {
            return this->profiles.at(index.row()).getName();
        }
        return QVariant();
    }

    void loadProfiles(const QDir& saveDir)
    {
        beginResetModel();
        this->profiles.clear();
        QDir profilesDir(saveDir.filePath("profiles"));
        if (profilesDir.exists()) {
            for(QFileInfo profileDirInfo: profilesDir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot)){
                if(profileDirInfo.fileName() == "0")
                    continue;
                QDir profileDir(profileDirInfo.absoluteFilePath());
                Profile profile(profileDir);
                if (profile.isValid()) {
                    this->profiles.append(profile);
                }
            }
        }
        endResetModel();
    }

    Profile& profileAt(const QModelIndex& index)
    {
        return this->profiles[index.row()];
    }

    void renameProfile(const QModelIndex& index, const QString& newName)
    {
        Profile& profile = this->profileAt(index);
        profile.setName(newName);
        emit dataChanged(index, index);
    }

    void deleteProfile(const QModelIndex& index)
    {
        beginRemoveRows(QModelIndex(), index.row(), index.row());
        Profile& profile = this->profileAt(index);
        profile.deleteFromDisk();
        this->profiles.removeAt(index.row());
        endRemoveRows();
    }

    void addProfile(const QDir& newProfileDir, const QString& newName)
    {
        beginInsertRows(QModelIndex(), this->profiles.size(), this->profiles.size());
        Profile newProfile(newProfileDir);
        newProfile.setName(newName);
        this->profiles.append(newProfile);
        endInsertRows();
    }

    Profile forName(const QString& name) const
    {
        for(const Profile &profile: this->profiles) {
            if (profile.getName() == name) {
                return profile;
            }
        }
        return Profile();
    }

    bool isDuplicated(const QString& name) const
    {
        for(const Profile &profile: this->profiles) {
            if (profile.getName() == name) {
                return true;
            }
        }
        return false;
    }

private:
    QList<Profile> profiles;
};



ChangeItWidget::ChangeItWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChangeItWidget)
{
    ui->setupUi(this);
    this->profileModel = new ProfileModel();
    this->proxyModel = new QSortFilterProxyModel(this);
    this->proxyModel->setSourceModel(this->profileModel);
    ui->lstProfiles->setModel(this->proxyModel);
    connect(ui->btnCreate, SIGNAL(clicked()), SLOT(addProfile()));
    connect(ui->btnRename, SIGNAL(clicked()), SLOT(renameProfile()));
    connect(ui->btnDelete, SIGNAL(clicked()), SLOT(deleteProfile()));
    connect(ui->lstProfiles, SIGNAL(activated(QModelIndex)), SLOT(activateProfile()));
    connect(ui->btnSelectSaveDir, SIGNAL(clicked()), SLOT(selectSaveDir()));
    connect(ui->btnReloadProfiles, SIGNAL(clicked()), SLOT(reloadProfiles()));
    reloadSettings();
    updateSysName();
}


ChangeItWidget::~ChangeItWidget()
{
    delete ui;
    delete proxyModel;
    delete profileModel;
}


void ChangeItWidget::showEvent(QShowEvent *)
{
    if (!checkSaveDir(false)) {
        QTimer::singleShot(0, this, SLOT(selectSaveDir()));
    }
}


bool ChangeItWidget::checkSaveDir(bool showWarning)
{
    QDir sysDir(saveDir.filePath("sys"));
    if (!sysDir.exists()) {
        if (showWarning) {
            QMessageBox::information(this, tr("Select Game Save Directory"),
                                     tr("The directory does not contains \"sys\" sub-directory. Please select another one."));
        }
        return false;
    } else {
        return true;
    }
}

QFileInfo configFileInfo()
{
    QDir configFileDir = QDir(QCoreApplication::applicationDirPath());
    if (!configFileDir.exists()) {
        configFileDir = QDir::current();
    }
    QFileInfo cfi(configFileDir, "config.ini");
    return cfi;
}

void ChangeItWidget::reloadSettings()
{
    QFileInfo cfi = configFileInfo();
    if (!cfi.exists()) {
        return;
    }
    QSettings settings(cfi.filePath(), QSettings::IniFormat);
    settings.setIniCodec("gbk");
    settings.beginGroup("dirs");
    const QString &saveDirPath  = settings.value("save").toString();
    saveDir = QDir(saveDirPath);
    qDebug() << "save dir is" << saveDir.path();
    if (checkSaveDir(false)) {
        profileModel->loadProfiles(saveDir);
    }
}


void ChangeItWidget::reloadProfiles()
{
    if (checkSaveDir(true)) {
        profileModel->loadProfiles(saveDir);
    }
}


void ChangeItWidget::saveSettings()
{
    QFileInfo cfi = configFileInfo();
    QSettings settings(cfi.filePath(), QSettings::IniFormat);
    settings.setIniCodec("gbk");
    settings.beginGroup("dirs");
    settings.setValue("save", saveDir.absolutePath());
}


void ChangeItWidget::selectSaveDir()
{
    QString oldDir;
    if (saveDir.exists()) {
        oldDir = saveDir.path();
    }
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Select Game Save Directory"),
                                                        oldDir, QFileDialog::ShowDirsOnly);
    if (dirPath.isEmpty()) {
        return;
    }
    this->saveDir = QDir(dirPath);
    if (checkSaveDir(true)) {
        saveSettings();
        this->profileModel->loadProfiles(saveDir);
    }
}


bool copyProfile(const QDir& source, const QDir& target, bool skipIni)
{
    QStringList fileList;
    bool failed = false;
    fileList << "info.ini" << "winsys.dxb" << "winsys.dxg";
    for (const QString &filename: fileList) {
        QString sourcePath = source.filePath(filename);
        if (!QFile::exists(sourcePath)) {
            if (filename == "info.ini" && skipIni) {
                // pass
            } else {
                qDebug() << "source file" << sourcePath << "is not found.";
                failed = true;
            }
            continue;
        }
        QString targetPath = target.filePath(filename);
        if(QFile::exists(targetPath))
            QFile::remove(targetPath);
        if (!QFile::copy(sourcePath, targetPath)) {
            qDebug() << "can not wirte to" << targetPath;
            failed = true;
        }
    }
    return !failed;
}

bool initProfile(const QDir &target, const QString &name)
{
    QByteArray data;
    qint64 writtenBytes;

    QFile winsys_dxb(":/winsys.dxb");
    if (!winsys_dxb.open(QIODevice::ReadOnly)) {
        qDebug() << "can not open internal winsys.dxb";
        return false;
    }
    QFile winsys_dxg(":/winsys.dxg");
    if (!winsys_dxg.open(QIODevice::ReadOnly)) {
        qDebug() << "can not open internal winsys.dxg";
        return false;
    }

    QFile winsys_dxb_new(target.filePath("winsys.dxb"));
    if (!winsys_dxb_new.open(QIODevice::WriteOnly)) {
        qDebug() << "can not open target winsys.dxb";
        return false;
    }
    data = winsys_dxb.readAll();
    if (data.isEmpty()) {
        qDebug() << "can not read from internal winsys.dxb";
        return false;
    }
    writtenBytes = winsys_dxb_new.write(data);
    if (writtenBytes != data.size()) {
        qDebug() << "can not write target winsys.dxb";
        return false;
    }
    winsys_dxb_new.close();

    QFile winsys_dxg_new(target.filePath("winsys.dxg"));
    if (!winsys_dxg_new.open(QIODevice::WriteOnly)) {
        qDebug() << "can not open target winsys.dxg";
        return false;
    }
    data = winsys_dxg.readAll();
    if (data.isEmpty()) {
        qDebug() << "can not read from internal winsys.dxg";
        return false;
    }
    writtenBytes = winsys_dxg_new.write(data);
    if (writtenBytes != data.size()) {
        qDebug() << "can not write target winsys.dxg";
        return false;
    }
    winsys_dxg_new.close();

    QString infoFile = target.filePath("info.ini");
    QSettings settings(infoFile, QSettings::IniFormat);
    settings.setIniCodec("gbk");
    settings.setValue("name", name);

    return true;
}


bool ChangeItWidget::createProfile(const QString &name, QDir *profileDir)
{
    QString newDirName = makeProfilePath();
    QDir profilesDir(saveDir.filePath("profiles"));
    if(!profilesDir.exists()) {
        saveDir.mkdir("profiles");
    }
    profilesDir.mkdir(newDirName);

    QDir newProfileDir(profilesDir.filePath(newDirName));
    if (initProfile(newProfileDir, name)) {
        if (profileDir) {
            *profileDir = newProfileDir;
        }
        return true;
    } else {
        return false;
    }
}


void ChangeItWidget::addProfile()
{
    if (!checkSaveDir(true)) {
        return;
    }
    bool okay;
    QString newName = QInputDialog::getText(this, tr("Create New Profile"),
                                            tr("Input the name of new profile:"),
                                            QLineEdit::Normal, QString(), &okay);
    if(!okay)
        return;
    if(this->profileModel->isDuplicated(newName)) {
        QMessageBox::information(this, tr("Create New Profile"), tr("The name of new profile is duplicated."));
        return;
    }

    QDir profileDir;
    if (!createProfile(newName, &profileDir)) {
        QMessageBox::information(this, tr("Create New Profile"), tr("Can not initialize profile. Check the game dir please."));
        return;
    }
    this->profileModel->addProfile(profileDir, newName);
}


void ChangeItWidget::renameProfile()
{
    const QModelIndex& current = ui->lstProfiles->currentIndex();
    if(!current.isValid())
        return;
    Profile& profile = this->profileModel->profileAt(this->proxyModel->mapToSource(current));
    bool okay;
    const QString &message = tr("Input the new name of profile \"%1\"").arg(profile.getName());
    QString newName = QInputDialog::getText(this, tr("Rename Profile"), message, QLineEdit::Normal, profile.getName(), &okay);
    if(!okay)
        return;

    QDir sysDir(saveDir.filePath("sys"));
    Profile mainProfile(sysDir);
    QString mainProfileName = mainProfile.getName();
    if (mainProfileName.isEmpty()) {
        mainProfileName = tr("Orignal Profile");
    }
    if (mainProfileName == profile.getName()) {
        mainProfile.setName(newName);
    }
    ui->lblSysName->setText(trUtf8("Current Profile: %1").arg(newName));
    this->profileModel->renameProfile(this->proxyModel->mapToSource(current), newName);
}


void ChangeItWidget::deleteProfile()
{
    const QModelIndex& current = ui->lstProfiles->currentIndex();
    if(!current.isValid())
        return;
    int answer = QMessageBox::question(this, tr("Remove Profile"),
                                       tr("This operation can not be reverted. Do you confirmed?"), QMessageBox::Yes | QMessageBox::Cancel);
    if(answer == QMessageBox::Cancel)
        return;
    this->profileModel->deleteProfile(this->proxyModel->mapToSource(current));
}


void ChangeItWidget::activateProfile()
{
    const QModelIndex& current = ui->lstProfiles->currentIndex();
    if(!current.isValid())
        return;

    QDir sysDir(saveDir.filePath("sys"));
    Profile mainProfile(sysDir);
    QString mainProfileName = mainProfile.getName();
    if (mainProfileName.isEmpty()) {
        mainProfileName = tr("Original Profile");
    }

    Profile oldProfile = this->profileModel->forName(mainProfileName);
    Profile& newProfile = this->profileModel->profileAt(this->proxyModel->mapToSource(current));
    qDebug() << mainProfile.getProfileDir() << oldProfile.getProfileDir() << newProfile.getProfileDir();

    if(!oldProfile.isValid()) {
        QDir oldProfileDir;
        if (!createProfile(mainProfileName, &oldProfileDir)) {
            QMessageBox::information(this, tr("Activate Profile"), tr("Can not activate profile. Failed to initialize profile. Check the game directory please."));
            return;
        }
        this->profileModel->addProfile(oldProfileDir, mainProfileName);
        oldProfile = this->profileModel->forName(mainProfileName);
    }
    bool success = copyProfile(mainProfile.getProfileDir(), oldProfile.getProfileDir(), true);
    if (!success) {
        QMessageBox::information(this, tr("Active Profile"), tr("Can not activate profile. Failed to copy files. Check the game save directory please."));
        return;
    }
    success = copyProfile(newProfile.getProfileDir(), mainProfile.getProfileDir(), false);
    if (!success) {
        QMessageBox::information(this, tr("Active Profile"), tr("Can not activate profile. Failed to copy files. Check the game save directory please."));
    }
    updateSysName();
}

void ChangeItWidget::updateSysName()
{
    QDir sysDir(saveDir.filePath("sys"));
    Profile mainProfile(sysDir);
    QString mainProfileName = mainProfile.getName();
    if (mainProfileName.isEmpty()) {
        mainProfileName = tr("Orignal Profile");
    }
    ui->lblSysName->setText(trUtf8("Current Profile: %1").arg(mainProfileName));
}
