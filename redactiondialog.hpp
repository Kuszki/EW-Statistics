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

#ifndef REDACTIONDIALOG_HPP
#define REDACTIONDIALOG_HPP

#include <QDialog>

namespace Ui
{
	class RedactionDialog;
}

class RedactionDialog : public QDialog
{

		Q_OBJECT

	private:

		Ui::RedactionDialog* ui;

	public:

		explicit RedactionDialog(QWidget* Parent = nullptr);
		virtual ~RedactionDialog(void) override;

	public slots:

		virtual void accept(void) override;

		void setParameters(const QVector<double>& Scales,
					    const QStringList& Exclude,
					    bool computeSymbols);

	signals:

		void onDialogAccepted(const QVector<double>& Scales,
						  const QStringList& Exclude,
						  bool computeSymbols);

};

#endif // REDACTIONDIALOG_HPP
