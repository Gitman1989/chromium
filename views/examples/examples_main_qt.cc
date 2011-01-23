#include "views/widget/widget_qt.h"

int main(int argc, char** argv) {
  // The exit manager is in charge of calling the dtors of singleton objects.
  base::AtExitManager exit_manager;

  views::WidgetQt w(views::WidgetQt::TYPE_WINDOW);
  return 0;
}
