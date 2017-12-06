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

#include "paymentwidget.hpp"
#include "ui_paymentwidget.h"

PaymentWidget::PaymentWidget(ApplicationCore* App, QWidget* Parent)
: QWidget(Parent), Core(App), ui(new Ui::PaymentWidget)
{
	static const QStringList defaultGroups =
	{
		tr("User"), tr("Month"), tr("Day")
	};

	ui->setupUi(this); QSet<QString> Used;

	ui->printButton->setEnabled(false);
	ui->previewButton->setEnabled(false);

	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Payments");
	startDate = Settings.value("start", QDate::currentDate()).toDate();
	stopDate = Settings.value("stop", QDate::currentDate()).toDate();
	singlePayment = Settings.value("payment", 15.0).toDouble();
	iddleDelay = Settings.value("delay", 15).toInt();

	QStringList List = Settings.value("groups").toStringList();
	QVariantList On = Settings.value("enabled").toList();

	Settings.endGroup();

	for (int i = 0; i < List.size() && i < On.size(); ++i)
	{
		if (!Used.contains(List[i]) && defaultGroups.contains(List[i]))
		{
			allGroups.append({ List[i], On[i].toBool() }); Used.insert(List[i]);
		}
	}

	for (const auto& Key : defaultGroups) if (!Used.contains(Key))
	{
		allGroups.append({ Key, false }); Used.insert(Key);
	}

	connect(this, &PaymentWidget::onDataReloaded, this, &PaymentWidget::recordsDataLoaded);
}

PaymentWidget::~PaymentWidget(void)
{
	QStringList Groups; QVariantList On;

	for (const auto& Group : allGroups)
	{
		Groups.append(Group.first);
		On.append(Group.second);
	}

	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Payments");
	Settings.setValue("start", startDate);
	Settings.setValue("stop", stopDate);
	Settings.setValue("payment", singlePayment);
	Settings.setValue("delay", iddleDelay);
	Settings.setValue("groups", Groups);
	Settings.setValue("enabled", On);
	Settings.endGroup();

	delete ui;
}

QHash<QString, QVector<QPair<QDateTime, PaymentWidget::ACTION>>> PaymentWidget::loadEvents(QSqlDatabase& Db)
{
	auto isDateBetween = [] (const QDateTime& D, const QDate& A, const QDate& B)
	{
		const QDate& C = D.date(); return (A <= C) && (B >= C);
	};

	QSqlQuery selectExistingObjects(Db),
			selectExistingTexts(Db),
			selectExistingLines(Db);

	QSqlQuery selectUsers(Db), selectObjects(Db);
	QSqlQuery selectTexts(Db), selectLines(Db);

	QSet<int> Objects, Texts, Lines;
	QHash<int, QString> Users;

	QHash<int, QList<QPair<QDateTime, ACTION>>> Events;
	QHash<QString, QVector<QPair<QDateTime, ACTION>>> Finall;

	selectExistingObjects.prepare("SELECT ID FROM EW_OBIEKTY WHERE STATUS = 0");
	selectExistingTexts.prepare("SELECT ID FROM EW_TEXT WHERE STAN_ZMIANY = 0");
	selectExistingLines.prepare("SELECT ID FROM EW_POLYLINE WHERE STAN_ZMIANY = 0");

	if (selectExistingObjects.exec()) while (selectExistingObjects.next())
	{
		Objects.insert(selectExistingObjects.value(0).toInt());
	}

	if (selectExistingTexts.exec()) while (selectExistingTexts.next())
	{
		Texts.insert(selectExistingTexts.value(0).toInt());
	}

	if (selectExistingLines.exec()) while (selectExistingLines.next())
	{
		Lines.insert(selectExistingLines.value(0).toInt());
	}

	selectUsers.prepare("SELECT ID, NAME FROM EW_USERS ORDER BY ID ASCENDING");

	if (selectUsers.exec()) while (selectUsers.next())
	{
		const int ID = selectUsers.value(0).toInt();

		Users.insert(ID, selectUsers.value(1).toString());

		Events.insert(ID, QList<QPair<QDateTime, ACTION>>());
	}

	selectObjects.prepare(
		"SELECT "
			"ID, "
			"OSOU, OSOW, OSOR, "
			"DTU, DTW, DTR "
		"FROM "
			"EW_OBIEKTY "
		"WHERE "
			"DTU BETWEEN :from AND :to OR "
			"DTW BETWEEN :from AND :to OR "
			"DTR BETWEEN :from AND :to");

	selectObjects.bindValue(":from", startDate);
	selectObjects.bindValue(":to", stopDate.addDays(1));

	selectTexts.prepare(
		"SELECT "
			"ID, TYP, "
			"USER_CREATE, USER_MODIFY, USER_DELETE, "
			"CREATE_TS, MODIFY_TS, DELETE_TS "
		"FROM "
			"EW_TEXT "
		"WHERE "
			"CREATE_TS BETWEEN :from AND :to OR "
			"MODIFY_TS BETWEEN :from AND :to OR "
			"DELETE_TS BETWEEN :from AND :to");

	selectTexts.bindValue(":from", startDate);
	selectTexts.bindValue(":to", stopDate.addDays(1));

	selectLines.prepare(
		"SELECT "
			"ID, "
			"USER_CREATE, USER_MODIFY, USER_DELETE, "
			"CREATE_TS, MODIFY_TS, DELETE_TS "
		"FROM "
			"EW_POLYLINE "
		"WHERE "
			"CREATE_TS BETWEEN :from AND :to OR "
			"MODIFY_TS BETWEEN :from AND :to OR "
			"DELETE_TS BETWEEN :from AND :to");

	selectLines.bindValue(":from", startDate);
	selectLines.bindValue(":to", stopDate.addDays(1));

	if (selectObjects.exec()) while (selectObjects.next())
	{
		const int ID = selectObjects.value(0).toInt();

		const int uADD = selectObjects.value(1).toInt();
		const int uMOD = selectObjects.value(2).toInt();
		const int uDEL = selectObjects.value(3).toInt();

		const QVariant dADD = selectObjects.value(4);
		const QVariant dMOD = selectObjects.value(5);
		const QVariant dDEL = selectObjects.value(6);

		if (dADD == dMOD)
		{
			const QDateTime DATE = dADD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uADD].append({ DATE, ADD_OBJECT });
			}
		}
		else if (dDEL.isNull())
		{
			const QDateTime DATE = dMOD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uMOD].append({ DATE, MOD_OBJECT });
			}
		}
		else if (!Objects.contains(ID))
		{
			const QDateTime DATE = dDEL.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uDEL].append({ DATE, DEL_OBJECT });
			}
		}
	}

	if (selectTexts.exec()) while (selectTexts.next())
	{
		const int ID = selectTexts.value(0).toInt();
		const bool Text = selectTexts.value(1).toInt() != 4;

		const int uADD = selectTexts.value(2).toInt();
		const int uMOD = selectTexts.value(3).toInt();
		const int uDEL = selectTexts.value(4).toInt();

		const QVariant dADD = selectTexts.value(5);
		const QVariant dMOD = selectTexts.value(6);
		const QVariant dDEL = selectTexts.value(7);

		if (dADD == dMOD)
		{
			const QDateTime DATE = dADD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uADD].append({ DATE, Text ? ADD_TEXT : ADD_SEGMENT });
			}
		}
		else if (dDEL.isNull())
		{
			const QDateTime DATE = dMOD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uMOD].append({ DATE, Text ? MOD_TEXT : MOD_SEGMENT });
			}
		}
		else if (!Objects.contains(ID))
		{
			const QDateTime DATE = dDEL.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uDEL].append({ DATE, Text ? DEL_TEXT : DEL_SEGMENT });
			}
		}
	}

	if (selectLines.exec()) while (selectLines.next())
	{
		const int ID = selectLines.value(0).toInt();

		const int uADD = selectLines.value(1).toInt();
		const int uMOD = selectLines.value(2).toInt();
		const int uDEL = selectLines.value(3).toInt();

		const QVariant dADD = selectLines.value(4);
		const QVariant dMOD = selectLines.value(5);
		const QVariant dDEL = selectLines.value(6);

		if (dADD == dMOD)
		{
			const QDateTime DATE = dADD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uADD].append({ DATE, ADD_SEGMENT });
			}
		}
		else if (dDEL.isNull())
		{
			const QDateTime DATE = dMOD.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uMOD].append({ DATE, MOD_SEGMENT });
			}
		}
		else if (!Objects.contains(ID))
		{
			const QDateTime DATE = dDEL.toDateTime();

			if (isDateBetween(DATE, startDate, stopDate))
			{
				Events[uDEL].append({ DATE, DEL_SEGMENT });
			}
		}
	}

	for (auto i = Events.constBegin(); i != Events.constEnd(); ++i)
	{
		Finall.insert(Users[i.key()], i.value().toVector());
	}

	return Finall;
}

void PaymentWidget::appendCounters(PaymentWidget::RECORD& Record, PaymentWidget::ACTION Action)
{
	switch (Action)
	{
		case ADD_OBJECT: ++Record.ObjectsADD; break;
		case MOD_OBJECT: ++Record.ObjectsMOD; break;
		case DEL_OBJECT: ++Record.ObjectsDEL; break;

		case ADD_TEXT: ++Record.TextsADD; break;
		case MOD_TEXT: ++Record.TextsMOD; break;
		case DEL_TEXT: ++Record.TextsDEL; break;

		case ADD_SEGMENT: ++Record.SegmentsADD; break;
		case MOD_SEGMENT: ++Record.SegmentsMOD; break;
		case DEL_SEGMENT: ++Record.SegmentsDEL; break;
	}
}

void PaymentWidget::appendCounters(PaymentWidget::STATPART& Record, const PaymentWidget::RECORD& Action)
{
	const unsigned Time = Action.Start.secsTo(Action.Stop);

	if (Record.Start.isNull()) Record.Start = Action.Start;
	if (Record.Stop.isNull()) Record.Stop = Action.Stop;

	Record.ObjectsADD += Action.ObjectsADD;
	Record.ObjectsMOD += Action.ObjectsMOD;
	Record.ObjectsDEL += Action.ObjectsDEL;

	Record.TextsADD += Action.TextsADD;
	Record.TextsMOD += Action.TextsMOD;
	Record.TextsDEL += Action.TextsDEL;

	Record.SegmentsADD += Action.SegmentsADD;
	Record.SegmentsMOD += Action.SegmentsMOD;
	Record.SegmentsDEL += Action.SegmentsDEL;

	Record.Start = qMin(Record.Start, Action.Start);
	Record.Stop = qMax(Record.Stop, Action.Stop);

	Record.Time += Time;
}

QString PaymentWidget::formatInfo(const PaymentWidget::STATPART& Record, double perHour)
{
	const QString Time = QString("%1:%2:%3")
					 .arg(Record.Time / 3600, 2, 10, QChar('0'))
					 .arg((Record.Time / 60) % 60, 2, 10, QChar('0'))
					 .arg(Record.Time % 60, 2, 10, QChar('0'));

	const QString Payment = QString::number((Record.Time / 3600.0) * perHour, 'f', 2);

	const unsigned Days = Record.Start.daysTo(Record.Stop);
	const unsigned Avg = Record.Time / (Days ? Days : 1);

	const QString Daily = QString("%1:%2:%3")
					  .arg(Avg / 3600, 2, 10, QChar('0'))
					  .arg((Avg / 60) % 60, 2, 10, QChar('0'))
					  .arg(Avg % 60, 2, 10, QChar('0'));

	return tr(
		"<h2>User: %1</h2>"
		"<ul>"
		"<li>Added objects: %2</li>"
		"<li>Modified objects: %3</li>"
		"<li>Removed objects: %4</li>"
		"</ul>"
		"<ul>"
		"<li>Added segments: %5</li>"
		"<li>Modified segments: %6</li>"
		"<li>Removed segments: %7</li>"
		"</ul>"
		"<ul>"
		"<li>Added texts: %8</li>"
		"<li>Modified texts: %9</li>"
		"<li>Removed texts: %10</li>"
		"</ul>"
		"<h4>Total time: %11</h4>"
		"<h4>Total payment: %12 PLN</h4>"
		"<h4>Avg per day: %13</h4>")
			.arg(Record.User)
			.arg(Record.ObjectsADD)
			.arg(Record.ObjectsMOD)
			.arg(Record.ObjectsDEL)
			.arg(Record.SegmentsADD)
			.arg(Record.SegmentsMOD)
			.arg(Record.SegmentsDEL)
			.arg(Record.TextsADD)
			.arg(Record.TextsMOD)
			.arg(Record.TextsDEL)
			.arg(Time)
			.arg(Payment)
			.arg(Daily);
}

QString PaymentWidget::tabledInfo(const PaymentWidget::STATPART& Record, double perHour)
{
	const double Hours = Record.Time / 3600.0;

	const QString Time = QString("%1:%2:%3")
					 .arg(Record.Time / 3600, 2, 10, QChar('0'))
					 .arg((Record.Time / 60) % 60, 2, 10, QChar('0'))
					 .arg(Record.Time % 60, 2, 10, QChar('0'));

	const QString Payment = QString::number(Hours * perHour, 'f', 2);

	const unsigned Days = Record.Start.daysTo(Record.Stop);
	const unsigned Avg = Record.Time / (Days ? Days : 1);

	const QString Daily = QString("%1:%2:%3")
					  .arg(Avg / 3600, 2, 10, QChar('0'))
					  .arg((Avg / 60) % 60, 2, 10, QChar('0'))
					  .arg(Avg % 60, 2, 10, QChar('0'));

	return tr(
		"<table border='0' width='100%' cellspacing='00' valign='middle'>"
		"<tr>"
		"<td><h2>User: %1</h2></td>"
		"<td><h4>Total payment: %12 PLN</h4></td>"
		"<td><h4>Total time: %11</h4></td>"
		"<td><h4>Avg time per day: %13</h4></td>"
		"</tr>"
		"</table>"
		"<table border='0' width='100%' cellspacing='15' valign='middle'>"
		"<tr>"
		"<td>"
		"<ul>"
		"<li>Added objects: <b>%2</b></li>"
		"<li>Modified objects: <b>%3</b></li>"
		"<li>Removed objects: <b>%4</b></li>"
		"<li><p>Avg per hour: <b>%14</b></p></li>"
		"</ul>"
		"</td>"
		"<td>"
		"<ul>"
		"<li>Added segments: <b>%5</b></li>"
		"<li>Modified segments: <b>%6</b></li>"
		"<li>Removed segments: <b>%7</b></li>"
		"<li><p>Avg per hour: <b>%15</b></p></li>"
		"</ul>"
		"</td>"
		"<td>"
		"<ul>"
		"<li>Added texts: <b>%8</b></li>"
		"<li>Modified texts: <b>%9</b></li>"
		"<li>Removed texts: <b>%10</b></li>"
		"<li><p>Avg per hour: <b>%16</b></p></li>"
		"</ul>"
		"</td>"
		"</tr>"
		"</table>")
			.arg(Record.User)
			.arg(Record.ObjectsADD)
			.arg(Record.ObjectsMOD)
			.arg(Record.ObjectsDEL)
			.arg(Record.SegmentsADD)
			.arg(Record.SegmentsMOD)
			.arg(Record.SegmentsDEL)
			.arg(Record.TextsADD)
			.arg(Record.TextsMOD)
			.arg(Record.TextsDEL)
			.arg(Time)
			.arg(Payment)
			.arg(Daily)
			.arg(int((Record.ObjectsADD + Record.ObjectsMOD + Record.ObjectsDEL) / Hours))
			.arg(int((Record.SegmentsADD + Record.SegmentsMOD + Record.SegmentsDEL) / Hours))
			.arg(int((Record.TextsADD + Record.TextsMOD + Record.TextsDEL) / Hours));
}

QString PaymentWidget::formatInfo(const QList<PaymentWidget::RECORD>& Records, double perHour)
{
	QString Output;

	Output.append("<table border='5' width='100%' cellspacing='0'>");

	Output.append(tr(
		"<tr>"
		"<th>Date</th>"
		"<th>From</th>"
		"<th>To</th>"
		"<th>Time</th>"
		"<th>Payment</th>"
		"</tr>"));

	for (const auto& R : Records)
	{
		const unsigned Time = R.Start.secsTo(R.Stop);

		const QString Formated = QString("%1:%2:%3")
							.arg(Time / 3600, 2, 10, QChar('0'))
							.arg((Time / 60) % 60, 2, 10, QChar('0'))
							.arg(Time % 60, 2, 10, QChar('0'));

		const double Payment = (Time / 3600.0) * perHour;

		Output.append(QString(
			"<tr>"
			"<td>%1</td>"
			"<td>%2</td>"
			"<td>%3</td>"
			"<td>%4</td>"
			"<td>%5 PLN</td>"
			"</tr>")
				    .arg(R.Start.date().toString(Qt::DefaultLocaleShortDate))
				    .arg(R.Start.toString(Qt::DefaultLocaleShortDate))
				    .arg(R.Stop.toString(Qt::DefaultLocaleShortDate))
				    .arg(Formated)
				    .arg(Payment, 0, 'f', 2));
	}

	Output.append("</table>");

	return Output;
}

void PaymentWidget::refreshButtonClicked(void)
{
	setEnabled(false); QtConcurrent::run(this, &PaymentWidget::refreshData);
}

void PaymentWidget::optionsButtonClicked(void)
{
	PaymentDialog* Dialog = new PaymentDialog(this);

	connect(Dialog, &PaymentDialog::accepted, Dialog, &PaymentDialog::deleteLater);
	connect(Dialog, &PaymentDialog::rejected, Dialog, &PaymentDialog::deleteLater);

	connect(Dialog, &PaymentDialog::onDialogAccepted, this, &PaymentWidget::setParameters);

	Dialog->setParameters(startDate, stopDate, allGroups, singlePayment, iddleDelay);
	Dialog->open();
}

void PaymentWidget::printButtonClicked(void)
{
	const bool Preview = sender() == ui->previewButton;

	const auto Selected = ui->treeView->selectionModel()->selectedRows();
	const auto Model = qobject_cast<RecordModel*>(ui->treeView->model());

	if (Selected.isEmpty()) { emit onDetailsRemove(); return; }

	const auto List = Model->getUids(Selected);

	QPrinter Printer(QPrinter::HighResolution);
	QPrintPreviewDialog PreviewDialog(&Printer, this);
	QPrintDialog PrintDialog(&Printer, this);

	QHash<QString, QList<RECORD>> Groups;
	QHash<QString, STATPART> Stats;
	QString Output;

	Output.reserve(Records.size() * 150);
	Printer.setFullPage(true);

	for (const auto& i : List)
	{
		const auto& R = Records[i];

		Stats[R.User].User = R.User;

		Groups[R.User].append(R);
		appendCounters(Stats[R.User], R);
	}

	QtConcurrent::blockingMap(Groups,
	[] (QList<RECORD>& List) -> void
	{
		std::stable_sort(List.begin(), List.end(),
		[] (const auto& A, const auto& B) -> bool
		{
			return A.Start < B.Start;
		});
	});

	Output.append(tr("<h1>Report from %1 to %2</h1><hr size='5'>")
			    .arg(startDate.toString())
			    .arg(stopDate.toString()));

	Output.append(tr("<p><b>Settings:</b> Max iddle time = %1 min; Payment per hour = %2 PLN</p>")
			    .arg(iddleDelay)
			    .arg(singlePayment, 0, 'f', 2));

	for (const auto& User : Stats.keys())
	{
		Output.append("<hr size='5'>");
		Output.append(tabledInfo(Stats[User], singlePayment));
		Output.append(formatInfo(Groups[User], singlePayment));
	}

	QTextDocument Doc; QFont Font = Doc.defaultFont();

	Font.setPointSize(9);

	Doc.setMetaInformation(QTextDocument::DocumentTitle, tr("Report"));
	Doc.setDefaultFont(Font);
	Doc.setHtml(Output);

	connect(&PreviewDialog, &QPrintPreviewDialog::paintRequested,
		   &Doc, &QTextDocument::print);

	if (Preview)
	{
		PreviewDialog.exec();
	}
	else if (PrintDialog.exec())
	{
		Doc.print(&Printer);
	}
}

void PaymentWidget::searchTextChanged(const QString& Text)
{
	auto Model = qobject_cast<RecordModel*>(ui->treeView->model());

	for (int i = 0; i < Records.size(); ++i)
	{
		const auto Index = Model->index(i);
		const auto Data = Model->fullData(Index);

		bool OK = Text.isEmpty();

		if (!OK) for (int j = 0; j < 2; ++j)
		{
			const QString Col = Data.value(j).toString();

			OK = OK || Col.contains(Text, Qt::CaseInsensitive);
		}

		ui->treeView->setRowHidden(Index.row(), Index.parent(), !OK);
	}
}

void PaymentWidget::recordsDataLoaded(const QList<PaymentWidget::RECORD>& Data)
{
	static const QStringList Headers =
	{
		tr("User"), tr("Day"), tr("Month"), tr("From"), tr("To"), tr("Hours"), tr("Payment")
	};

	RecordModel* Model = new RecordModel(Headers, this);
	QStringList enabledGroups;

	for (int i = 0; i < Data.size(); ++i)
	{
		const RECORD& Row = Data[i];
		QHash<int, QVariant> Record;

		const unsigned Time = Row.Start.secsTo(Row.Stop);

		const QString Formated = QString("%1:%2:%3")
							.arg(Time / 3600, 2, 10, QChar('0'))
							.arg((Time / 60) % 60, 2, 10, QChar('0'))
							.arg(Time % 60, 2, 10, QChar('0'));

		const double Payment = (Time / 3600.0) * singlePayment;

		Record[0] = Row.User;
		Record[1] = Row.Start.date();
		Record[2] = Row.Start.date().toString("MM.yyyy");
		Record[3] = Row.Start;
		Record[4] = Row.Stop;
		Record[5] = Formated;
		Record[6] = QString::number(Payment, 'f', 2).append(tr(" PLN"));

		Model->addItem(i, Record);
	}

	auto oldModel = ui->treeView->model();
	auto oldSelect = ui->treeView->selectionModel();

	ui->treeView->setModel(Model);

	for (const auto& Group : allGroups) if (Group.second)
	{
		enabledGroups.append(Group.first);
	}

	Model->groupByStr(enabledGroups);

	delete oldModel;
	delete oldSelect;

	setEnabled(true);

	Records = Data;

	connect(ui->treeView->selectionModel(),
		   &QItemSelectionModel::selectionChanged,
		   this, &PaymentWidget::selectionChanged);
}

void PaymentWidget::selectionChanged(void)
{
	const auto Selected = ui->treeView->selectionModel()->selectedRows();
	const auto Model = qobject_cast<RecordModel*>(ui->treeView->model());

	if (Selected.isEmpty())
	{
		ui->printButton->setEnabled(false);
		ui->previewButton->setEnabled(false);

		emit onDetailsRemove(); return;
	}
	else
	{
		ui->printButton->setEnabled(true);
		ui->previewButton->setEnabled(true);
	}

	const auto List = Model->getUids(Selected);

	QHash<QString, STATPART> Stats;
	QStringList Infos;

	for (const auto& i : List)
	{
		const auto& Rec = Records[i];
		appendCounters(Stats[Rec.User], Rec);
	}

	for (auto i = Stats.begin(); i != Stats.end(); ++i)
	{
		i.value().User = i.key();

		const QString Part = formatInfo(i.value(), singlePayment);

		Infos.append(Part);
	}

	emit onDetailsUpdate(Infos.join("<hr size='5'>"));
}

void PaymentWidget::refreshData(void)
{
	QMutexLocker Locker(&Synchronizer);

	QHash<QString, QVector<QPair<QDateTime, ACTION>>> Data;
	auto Databases = Core->getDatabases();
	QList<RECORD> Records;

	const int Secs = iddleDelay * 60;

	for (int i = 0; i < Databases.size(); ++i)
	{
		const auto Part = loadEvents(Databases[i]);

		for (auto j = Part.constBegin(); j != Part.constEnd(); ++j)
		{
			if (Data.contains(j.key()))
			{
				Data[j.key()].append(j.value());
			}
			else Data.insert(j.key(), j.value());
		}
	}

	QtConcurrent::blockingMap(Data,
	[] (QVector<QPair<QDateTime, ACTION>>& List) -> void
	{
		std::stable_sort(List.begin(), List.end());
	});

	for (auto i = Data.constBegin(); i != Data.constEnd(); ++i)
	{
		const auto& User = i.key();
		const auto& List = i.value();

		if (List.size() < 2) continue;

		RECORD Record = { User, List.first().first, List.first().first };

		for (const auto& Rec : List)
		{
			const int Diff = Record.Stop.secsTo(Rec.first);

			if (Diff < Secs)
			{
				appendCounters(Record, Rec.second);
				Record.Stop = Rec.first;
			}
			else
			{
				if (Record.Start.secsTo(Record.Stop))
				{
					Records.append(Record);
				}

				Record = { User, Rec.first, Rec.first };
			}
		}

		if (Record.Start.secsTo(Record.Stop)) Records.append(Record);
	}

	emit onDataReloaded(Records);
}

void PaymentWidget::setParameters(const QDate& Start, const QDate& Stop, const QList<QPair<QString, bool>>& Groups, double Payment, bool Refresh, int Delay)
{
	startDate = Start;
	stopDate = Stop;

	allGroups = Groups;

	singlePayment = Payment;
	iddleDelay = Delay;

	if (Refresh)
	{
		refreshButtonClicked();
	}
}
