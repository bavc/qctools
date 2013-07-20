#ifndef FILELIST_H
#define FILELIST_H

#include <QTableWidget>

/*namespace Ui {
class FileList;
}*/

class FileList : public QTableWidget
{
    Q_OBJECT
    
public:
    explicit FileList(QWidget *parent = 0);
    ~FileList();
    
private:
    //Ui::FileList *ui;
};

#endif // FILELIST_H
