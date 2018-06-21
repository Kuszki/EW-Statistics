/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  {description}                                                          *
 *  Copyright (C) 2018  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
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

#include "redactionwidget.hpp"
#include "ui_redactionwidget.h"

RedactionWidget::RedactionWidget(QWidget* Parent)
: QWidget(Parent), ui(new Ui::RedactionWidget)
{
	ui->setupUi(this);
}

RedactionWidget::~RedactionWidget(void)
{
	delete ui;
}

QHash<QString, QList<const RedactionWidget::FIELD*>> RedactionWidget::loadTables(QSqlDatabase& Db, const QHash<int, FIELD>& Fields)
{

}

QHash<int, QVector<double>> RedactionWidget::loadLayers(QSqlDatabase& Db)
{

}

QHash<int, RedactionWidget::FIELD> RedactionWidget::loadfields(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true);

	Query.prepare()
}
