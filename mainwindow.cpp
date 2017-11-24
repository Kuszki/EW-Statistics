/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  {description}                                                          *
 *  Copyright (C) 2017  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the  Free Software Foundation, either  version 3 of the  License, or   *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This  program  is  distributed  in the hope  that it will be useful,   *
 *  but WITHOUT ANY  WARRANTY;  without  even  the  implied  warranty of   *
 *  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have  received a copy  of the  GNU General Public License   *
 *  along with this program. If not, see http://www.gnu.org/licenses/.     *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* Parent)
: QMainWindow(Parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	Core = new ApplicationCore(this);

	About = new AboutDialog(this);

	connect(ui->actionAbout, &QAction::triggered, About, &AboutDialog::open);
	connect(ui->actionDatabases, &QAction::triggered, this, &MainWindow::databasesActionClicked);
}

MainWindow::~MainWindow(void)
{
	delete ui;
}

void MainWindow::databasesActionClicked(void)
{
	DatabasesDialog* Dialog = new DatabasesDialog(Core->getSavedDatabases(), this);

	connect(Dialog, &DatabasesDialog::onDialogAccepted, Core, &ApplicationCore::saveDatabasesList);

	connect(Dialog, &DatabasesDialog::accepted, Dialog, &DatabasesDialog::deleteLater);
	connect(Dialog, &DatabasesDialog::rejected, Dialog, &DatabasesDialog::deleteLater);

	Dialog->open();
}
