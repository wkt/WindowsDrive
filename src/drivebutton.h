#ifndef DRIVEBUTTON_H
#define DRIVEBUTTON_H


#include <QPushButton>
#include <map>
#include <string>
#include <QString>

namespace Ui {
class DriveButton;
}

class DriveButton : public QPushButton
{
    Q_OBJECT
    
public:
    explicit DriveButton(QWidget *parent = nullptr);
    ~DriveButton();

    void updateUiFrom(const std::map<std::string,std::string> &m);

    const QString& driveMountpoint() const;

public slots:
    void handleClicked();
    void showWindowsIcon(const bool& show=true);

private:
    Ui::DriveButton *ui;
    QString mp;
};

#endif // DRIVEBUTTON_H
