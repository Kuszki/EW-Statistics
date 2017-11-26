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

#include "databasesdialog.hpp"
#include "ui_databasesdialog.h"

DatabasesDialog::DatabasesDialog(const QList<DBINFO>& Databases, QWidget* Parent)
: QDialog(Parent), ui(new Ui::DatabasesDialog)
{
	static const QStringList Headers =
	{
		tr("Name"),
		tr("Server"),
		tr("Path")
	};

	ui->setupUi(this);

	Model = new QStandardItemModel(0, 3, this);

	Model->setHorizontalHeaderLabels(Headers);

	ui->tableView->setModel(Model);

	setDatabases(Databases);
}

DatabasesDialog::~DatabasesDialog(void)
{
	delete ui;
}

QList<DBINFO> DatabasesDialog::getDatabases(void) const
{
	QList<DBINFO> List, Finall;

	for (int i = 0; i < Model->rowCount(); ++i) List.append(
	{
		Model->item(i, 0)->data(Qt::EditRole).toString(),
		Model->item(i, 1)->data(Qt::EditRole).toString(),
		Model->item(i, 2)->data(Qt::EditRole).toString(),
		Model->item(i, 0)->checkState() == Qt::Checked,
	});

	for (const auto& Db : List) if (!Finall.contains(Db))
	{
		if (!Db.Path.isEmpty() && !Db.Server.isEmpty())
		{
			Finall.append(Db);
		}
	}

	return Finall;
}

void DatabasesDialog::accept(void)
{
	QDialog::accept(); emit onDialogAccepted(getDatabases());
}

void DatabasesDialog::setDatabases(const QList<DBINFO>& Databases)
{
	while (Model->rowCount()) Model->removeRow(0);

	for (const auto& Db : Databases)
	{
		auto Active = new QStandardItem(Db.Name);
		auto Server = new QStandardItem(Db.Server);
		auto Path = new QStandardItem(Db.Path);

		Active->setCheckable(true);
		Active->setCheckState(Db.Enabled ? Qt::Checked : Qt::Unchecked);

		Model->appendRow(QList<QStandardItem*>() << Active << Server << Path);
	}
}

void DatabasesDialog::addButtonClicked(void)
{
	auto Active = new QStandardItem();
	auto Server = new QStandardItem();
	auto Path = new QStandardItem();

	Active->setCheckable(true);
	Active->setCheckState(Qt::Unchecked);

	Model->appendRow(QList<QStandardItem*>() << Active << Server << Path);
}

void DatabasesDialog::removeButtonClicked(void)
{
	QVector<int> SortedRows; SortedRows.reserve(Model->rowCount());

	for (const auto& Row : ui->tableView->selectionModel()->selectedRows())
	{
		SortedRows.append(Row.row());
	}

	qSort(SortedRows); std::reverse(SortedRows.begin(), SortedRows.end());

	for (const auto& Row : SortedRows)
	{
		Model->removeRow(Row);
	}
}
