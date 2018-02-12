/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Compute various statistics for EWMAPA software                         *
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

#include "paymentdialog.hpp"
#include "ui_paymentdialog.h"

PaymentDialog::PaymentDialog(QWidget *Parent)
: QDialog(Parent), ui(new Ui::PaymentDialog)
{
	ui->setupUi(this);

	auto Model = new QStandardItemModel(this);

	ui->listView->setModel(Model);

	iddleSpinChanged(ui->iddleSpin->value());
}

PaymentDialog::~PaymentDialog(void)
{
	delete ui;
}

QList<QPair<QString, bool>> PaymentDialog::getGroups(void) const
{
	QStandardItemModel* Model = qobject_cast<QStandardItemModel*>(ui->listView->model());

	QList<QPair<QString, bool>> Items;

	for (int i = 0; i < Model->rowCount(); ++i)
	{
		const auto Item = Model->item(i);

		Items.append(
		{
			Item->text(),
			Item->checkState() == Qt::Checked
		});
	}

	return Items;
}

void PaymentDialog::accept(void)
{
	QDialog::accept();

	emit onDialogAccepted(
				ui->startDate->date(),
				ui->stopDate->date(),
				getGroups(),
				ui->paymentSpin->value(),
				false,
				ui->iddleSpin->value());
}

void PaymentDialog::setParameters(const QDate& Start, const QDate& Stop, const QList<QPair<QString, bool>>& Groups, double Payment, int Delay)
{
	QStandardItemModel* Model = qobject_cast<QStandardItemModel*>(ui->listView->model());

	ui->startDate->setMaximumDate(Stop);
	ui->stopDate->setMinimumDate(Start);

	ui->startDate->setDate(Start);
	ui->stopDate->setDate(Stop);

	ui->paymentSpin->setValue(Payment);
	ui->iddleSpin->setValue(Delay);

	while (Model->rowCount()) Model->removeRow(0);

	for (const auto& Group : Groups)
	{
		QStandardItem* Item = new QStandardItem(Group.first);

		Item->setCheckable(true);
		Item->setCheckState(Group.second ? Qt::Checked : Qt::Unchecked);
		Item->setDropEnabled(false);

		Model->appendRow(Item);
	}
}

void PaymentDialog::dialogButtonClicked(QAbstractButton* Button)
{
	const auto Save = ui->buttonBox->button(QDialogButtonBox::Apply);
	const auto This = qobject_cast<QPushButton*>(Button);

	if (Save != This) return;
	else QDialog::accept();

	emit onDialogAccepted(
				ui->startDate->date(),
				ui->stopDate->date(),
				getGroups(),
				ui->paymentSpin->value(),
				true,
				ui->iddleSpin->value());
}

void PaymentDialog::startDateChanged(const QDate& Date)
{
	ui->stopDate->setMinimumDate(Date);
}

void PaymentDialog::stopDateChanged(const QDate& Date)
{
	ui->startDate->setMaximumDate(Date);
}

void PaymentDialog::iddleSpinChanged(int Value)
{
	ui->iddleSpin->setSuffix(tr(" minute(s)", nullptr, Value));
}
