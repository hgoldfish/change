#ifndef CHANGEITWIDGET_H
#define CHANGEITWIDGET_H

#include <QWidget>
#include <QDir>
#include <QSortFilterProxyModel>

namespace Ui {
class ChangeItWidget;
}

class ProfileModel;

class ChangeItWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChangeItWidget(QWidget *parent = 0);
    ~ChangeItWidget();

protected:
    virtual void showEvent(QShowEvent *event) override;
private slots:
    void addProfile();
    void renameProfile();
    void deleteProfile();
    void activateProfile();
    void updateSysName();
    void selectSaveDir();
    void reloadProfiles();
private:
    void reloadSettings();
    void saveSettings();
    bool createProfile(const QString &name, QDir *profileDir);
    bool checkSaveDir(bool showWarning);
private:
    Ui::ChangeItWidget *ui;
    ProfileModel* profileModel;
    QSortFilterProxyModel *proxyModel;
    QDir saveDir;
};

#endif // CHANGEITWIDGET_H
