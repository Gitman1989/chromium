// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/examples/examples_main.h"

#include "app/app_paths.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/i18n/icu_util.h"
#include "base/process_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "views/controls/label.h"
#include "views/controls/button/text_button.h"
#include "views/examples/button_example.h"
#include "views/examples/combobox_example.h"
#include "views/examples/message_box_example.h"
#include "views/examples/menu_example.h"
#include "views/examples/radio_button_example.h"
#include "views/examples/scroll_view_example.h"
#include "views/examples/single_split_view_example.h"
// Slider is not yet ported to Windows.
#if defined(OS_LINUX)
#include "views/examples/slider_example.h"
#endif
#include "views/examples/tabbed_pane_example.h"
#if defined(OS_WIN)
// TableView is not yet ported to Linux.
#include "views/examples/table_example.h"
#endif
#include "views/examples/table2_example.h"
#include "views/examples/textfield_example.h"
#include "views/examples/throbber_example.h"
#include "views/examples/widget_example.h"
#include "views/focus/accelerator_handler.h"
#include "views/grid_layout.h"
#include "views/window/window.h"

#include <QtGui/QApplication>


namespace examples {

views::View* ExamplesMain::GetContentsView() {
  return contents_;
}

void ExamplesMain::WindowClosing() {
  MessageLoopForUI::current()->Quit();
}

void ExamplesMain::SetStatus(const std::wstring& status) {
  status_label_->SetText(status);
}

void ExamplesMain::Run() {
  base::EnableTerminationOnHeapCorruption();

  // The exit manager is in charge of calling the dtors of singleton objects.
  base::AtExitManager exit_manager;

  app::RegisterPathProvider();
  ui::RegisterPathProvider();

  icu_util::Initialize();

  ResourceBundle::InitSharedInstance("en-US");

  MessageLoop main_message_loop(MessageLoop::TYPE_UI);

  // Creates a window with the tabbed pane for each examples, and
  // a label to print messages from each examples.
  DCHECK(contents_ == NULL) << "Run called more than once.";
  contents_ = new views::View();
  contents_->set_background(views::Background::CreateStandardPanelBackground());
  views::GridLayout* layout = new views::GridLayout(contents_);
  contents_->SetLayoutManager(layout);

  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(views::GridLayout::FILL, views::GridLayout::FILL, 1,
                        views::GridLayout::USE_PREF, 0, 0);

  views::TabbedPane* tabbed_pane = new views::TabbedPane();
  status_label_ = new views::Label();

  layout->StartRow(1, 0);
  layout->AddView(tabbed_pane);
  layout->StartRow(0 /* no expand */, 0);
  layout->AddView(status_label_);

  // TODO(satorux): The window is getting wide.  Eventually, we would have
  // the second tabbed pane.

  views::Window* window =
      views::Window::CreateChromeWindow(NULL, gfx::Rect(0, 0, 850, 300), this);

  examples::TextfieldExample t(this);
  tabbed_pane->AddTab(t.GetExampleTitle(), t.GetExampleView());
  //tabbed_pane->SetVisible(false);

  examples::ButtonExample b(this);
  tabbed_pane->AddTab(b.GetExampleTitle(), b.GetExampleView());
  //tabbed_pane->set_background("Red");

  //views::RadioButton* radio_button = new views::RadioButton(L"push",0);
  //radio_button->radiobutton_->show();
  //radio_button->SetVisible(false);

  window->Show();

  views::AcceleratorHandler accelerator_handler;
  MessageLoopForUI::current()->Run(&accelerator_handler);
  //QApplication::exec();
}

}  // examples namespace

int main(int argc, char** argv) {
#if defined(OS_WIN)
  OleInitialize(NULL);
#elif defined(OS_LINUX)
  // Initializes gtk stuff.
  g_thread_init(NULL);
  g_type_init();
  gtk_init(&argc, &argv);
#endif

  QApplication app(argc, argv);

  CommandLine::Init(argc, argv);
  examples::ExamplesMain main;
  main.Run();



#if defined(OS_WIN)
  OleUninitialize();
#endif
  return 0;
}
