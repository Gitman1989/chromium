#include <QtGui/QApplication>
#include <QtGui/QMainWindow>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QMainWindow w;
    w.show();
    return a.exec();
}