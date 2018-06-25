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

#include "redactiondialog.hpp"
#include "ui_redactiondialog.h"

RedactionDialog::RedactionDialog(QWidget* Parent)
: QDialog(Parent), ui(new Ui::RedactionDialog)
{
	ui->setupUi(this);
}

RedactionDialog::~RedactionDialog(void)
{
	delete ui;
}

void RedactionDialog::accept(void)
{
	QDialog::accept(); const QVector<double> Scales =
	{
		ui->aScaleSpin->value(),
		ui->bScaleSpin->value(),
		ui->cScaleSpin->value(),
		ui->dScaleSpin->value()
	};

	const QStringList Exclude = ui->excludeEdit->toPlainText().split('\n', QString::SkipEmptyParts);

	emit onDialogAccepted(Scales, Exclude, ui->computeBox->isChecked());
}

void RedactionDialog::setParameters(const QVector<double>& Scales, const QStringList& Exclude, bool computeSymbols)
{
	ui->aScaleSpin->setValue(Scales.value(0, 0.5));
	ui->bScaleSpin->setValue(Scales.value(1, 1.0));
	ui->cScaleSpin->setValue(Scales.value(2, 2.0));
	ui->dScaleSpin->setValue(Scales.value(3, 5.0));

	ui->excludeEdit->setPlainText(Exclude.join('\n'));

	ui->computeBox->setChecked(computeSymbols);
}
