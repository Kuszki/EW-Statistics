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

#include "redactionwidget.hpp"
#include "ui_redactionwidget.h"

RedactionWidget::RedactionWidget(ApplicationCore* App, QWidget* Parent)
: QWidget(Parent), Core(App), ui(new Ui::RedactionWidget)
{
	ui->setupUi(this); QSettings Settings("EW-Statistics");

	Settings.beginGroup("Redaction");
	smbScales << Settings.value("smb500", 0.5).toDouble();
	smbScales << Settings.value("smb1000", 1.0).toDouble();
	smbScales << Settings.value("smb2000", 2.0).toDouble();
	smbScales << Settings.value("smb5000", 5.0).toDouble();
	smbExclude = Settings.value("exclude").toStringList();
	mapScale = SCALE(Settings.value("scale", 0).toInt());
	smbCompute = Settings.value("smbcmp", true).toBool();
	Settings.endGroup();

	ui->scaleCombo->setCurrentIndex(mapScale);

	connect(this, &RedactionWidget::onDataReloaded, this, &RedactionWidget::dataReloaded);
}

RedactionWidget::~RedactionWidget(void)
{
	QSettings Settings("EW-Statistics");

	Settings.beginGroup("Redaction");
	Settings.setValue("scale", mapScale);
	Settings.setValue("exclude", smbExclude);
	Settings.setValue("smb500", smbScales.value(0, 0.5));
	Settings.setValue("smb1000", smbScales.value(1, 1.0));
	Settings.setValue("smb2000", smbScales.value(2, 2.0));
	Settings.setValue("smb5000", smbScales.value(3, 5.0));
	Settings.setValue("smbcmp", smbCompute);
	Settings.endGroup();

	delete ui;
}

void RedactionWidget::setParameters(const QVector<double>& Scales, const QStringList& Exclude, bool computeSymbols)
{
	smbScales = Scales;
	smbExclude = Exclude;
	smbCompute = computeSymbols;
}

QList<RedactionWidget::TABLE> RedactionWidget::loadTables(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QList<TABLE> List;

	Query.prepare(
		"SELECT "
			"O.KOD, O.OPIS, O.DANE_DOD, O.OPCJE, "
			"F.NAZWA, F.TYTUL, F.TYP, "
			"D.WARTOSC, D.OPIS, "
			"BIN_AND(O.OPCJE, 266),"
			"S.WYPELNIENIE, D.SOPIS "
		"FROM "
			"EW_OB_OPISY O "
		"INNER JOIN "
			"EW_OB_DDSTR F "
		"ON "
			"O.KOD = F.KOD "
		"INNER JOIN "
			"EW_OB_DDSTR S "
		"ON "
			"F.KOD = S.KOD "
		"LEFT JOIN "
			"EW_OB_DDSL D "
		"ON "
			"S.UID = D.UIDP OR S.UIDSL = D.UIDP "
		"WHERE "
			"S.NAZWA = F.NAZWA "
		"ORDER BY "
			"O.KOD, F.NAZWA, D.OPIS");

	if (Query.exec()) while (Query.next())
	{
		const QString Field = Query.value(4).toString();
		const QString Table = Query.value(0).toString();
		const bool Dict = !Query.value(7).isNull();

		if (!hasItemByField(List, Table, &TABLE::Name)) List.append(
		{
			Table,
			Query.value(1).toString(),
			Query.value(2).toString(),
			bool(Query.value(3).toInt() & 0x100),
			Query.value(9).toInt()
		});

		auto& Tabref = getItemByField(List, Table, &TABLE::Name);

		if (!hasItemByField(Tabref.Fields, Field, &FIELD::Name)) Tabref.Fields.append(
		{
			TYPE(Query.value(6).toInt()),
			Field,
			Query.value(5).toString(),
			Query.value(10).toInt() == 2
		});

		auto& Fieldref = getItemByField(Tabref.Fields, Field, &FIELD::Name);

		if (Dict) Fieldref.Dict.insert(Query.value(7),
		{
			Query.value(8).toString(),
			Query.value(11).toString()
		});
	}

	Query.prepare("SELECT KOD, '${u.' || NAZWA || '}', DEFINICJA FROM EW_OB_ZMIENNE");

	if (Query.exec()) while (Query.next())
	{
		const QString Table = Query.value(0).toString();

		if (!hasItemByField(List, Table, &TABLE::Name)) continue;

		auto& Tabref = getItemByField(List, Table, &TABLE::Name);

		Tabref.Labels.insert(Query.value(1).toString(), Query.value(2).toString());
	}

	return List;
}

QHash<int, QVector<double>> RedactionWidget::loadLayers(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QHash<int, QVector<double>> List;

	Query.prepare("SELECT ID, WYSOKOSC, MNOZNIK_M_S500, MNOZNIK_M_S1000, MNOZNIK_M_S2000, MNOZNIK_M_S5000 FROM EW_WARSTWA_TEXTOWA");

	if (Query.exec()) while (Query.next()) List.insert(Query.value(0).toInt(), QVector<double>()
											 << Query.value(1).toDouble()
											 << Query.value(2).toDouble()
											 << Query.value(3).toDouble()
											 << Query.value(4).toDouble()
											 << Query.value(5).toDouble());

	return List;
}

QHash<int, QList<QVariant>> RedactionWidget::loadData(const RedactionWidget::TABLE& Table, QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true);
	QHash<int, QList<QVariant>> List; QStringList Attribs;

	for (const auto& Field : Table.Fields) Attribs.append(QString("EW_DATA.%1").arg(Field.Name));

	const auto append = [&] (QSqlQuery& Query, int Index) -> void
	{
		static QList<QVariant> Values; Values.reserve(Table.Fields.size());

		for (int j = 1; j <= Table.Fields.size(); ++j) Values.append(Query.value(j));

		if (!Values.isEmpty()) List.insert(Index, Values); Values.clear();
	};

	QString Exec = QString(
		"SELECT "
			"EW_OBIEKTY.UID, %1 "
		"FROM "
			"EW_OBIEKTY "
		"LEFT JOIN "
			"%2 EW_DATA "
		"ON "
			"EW_OBIEKTY.UID = EW_DATA.UIDO "
		"WHERE "
			"EW_OBIEKTY.KOD = '%3' AND "
			"EW_OBIEKTY.STATUS = 0")
				.arg(Attribs.join(", "))
				.arg(Table.Data)
				.arg(Table.Name);

	if (!Query.prepare(Exec)) return QHash<int, QList<QVariant>>();
	else if (Query.exec()) while (Query.next())
	{
		const int Index = Query.value(0).toInt(); append(Query, Index);
	}

	return List;
}

QHash<int, QList<RedactionWidget::LABEL>> RedactionWidget::loadLabels(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QHash<int, QList<LABEL>> List;

	Query.prepare(
		"SELECT "
			"O.UID, T.ID, T.POS_X, T.POS_Y, T.TEXT, "
			"T.ID_WARSTWY, T.JUSTYFIKACJA, T.KAT "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT T "
		"ON "
			"E.IDE = T.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 6");

	if (Query.exec()) while (Query.next()) List[Query.value(0).toInt()].append(
	{
		Query.value(1).toInt(),
		{
			Query.value(2).toDouble(),
			Query.value(3).toDouble()
		},
		Query.value(4).toString(),
		Query.value(5).toInt(),
		Query.value(6).toInt() & 0b1111,
		qRadiansToDegrees(Query.value(7).toDouble())
	});

	return List;
}

QList<QPointF> RedactionWidget::loadSymbols(QSqlDatabase& Db)
{
	QSqlQuery Query(Db); Query.setForwardOnly(true); QList<QPointF> List;

	Query.prepare(
		"SELECT "
			"T.TEXT, T.POS_X, T.POS_Y "
		"FROM "
			"EW_OBIEKTY O "
		"INNER JOIN "
			"EW_OB_ELEMENTY E "
		"ON "
			"O.UID = E.UIDO "
		"INNER JOIN "
			"EW_TEXT T "
		"ON "
			"E.IDE = T.ID "
		"WHERE "
			"O.STATUS = 0 AND "
			"E.TYP = 0 AND "
			"T.STAN_ZMIANY = 0 AND "
			"T.TYP = 4");

	if (Query.exec()) while (Query.next()) if (!smbExclude.contains(Query.value(0).toString())) List.append(
	{
		Query.value(1).toDouble(),
		Query.value(2).toDouble()
	});

	return List;
}

void RedactionWidget::parseLabels(QHash<int, QList<RedactionWidget::LABEL>>& Labels, const RedactionWidget::TABLE& Tab, const QHash<int, QList<QVariant>>& Values) const
{
	const auto parseLabel = [&Tab] (LABEL& Label, const QList<QVariant>& Values) -> void
	{
		QRegExp Exp("\\$\\{e\\.(\\w+)((?:,[^\\}]+)*)\\}"); int i(0); QStringList Str;

		if (Tab.Labels.contains(Label.Text)) Label.Text = Tab.Labels[Label.Text];

		while ((i = Exp.indexIn(Label.Text, i)) != -1)
		{
			const QStringList Cap = Exp.capturedTexts();

			const QString Var = Cap.value(1).remove("_SK");
			const bool SK = Cap.value(1).endsWith("_SK");

			const QStringList Args = Cap.value(2).split(',', QString::SkipEmptyParts);

			QStringList Not; for (const auto& Arg : Args) if (Arg.startsWith('N'))
			{
				Not.append(Arg.mid(1));
			}

			for (int j = 0; j < Tab.Fields.size(); ++j) if (Var == Tab.Fields[j].Name)
			{
				const auto Val = getDataFromDict(Values[j], Tab.Fields[j].Dict, Tab.Fields[j].Type, SK);

				if (isVariantEmpty(Val)) Str.append(QString());
				else if (Not.contains(Val.toString())) Str.append(QString());
				else Str.append(Val.toString());
			}

			i += Exp.matchedLength();
		}

		if (!Str.isEmpty()) Label.Text.replace(Exp, "$");

		while (Str.size() && (i = Label.Text.indexOf('$')) != -1)
		{
			Label.Text.replace(i, 1, Str.takeFirst());
		}
	};

	auto Keys = Labels.keys().toSet() & Values.keys().toSet();

	QtConcurrent::blockingMap(Keys, [&Labels, &Values, parseLabel] (int UID) -> void
	{
		for (auto& Label : Labels[UID]) parseLabel(Label, Values[UID]);
	});
}

void RedactionWidget::surfLabels(QHash<int, QList<RedactionWidget::LABEL>>& Labels, SCALE Sc, const QHash<int, QVector<double>>& Layers) const
{
	const auto maxSize = [] (const QString& Str) -> int
	{
		int Max = 0; auto List = Str.split('|', QString::SkipEmptyParts);

		for (const auto& L : List) Max = qMax(Max, L.size()); return Max;
	};

	const auto surfLabel = [Sc, maxSize] (LABEL& Label, const QVector<double>& Layer) -> void
	{
		const double dy = (2.0/3.0) * Layer[0] * Layer[int(Sc) + 1];
		const double dx = Layer[0] * Layer[int(Sc) + 1];

		const int lines = Label.Text.count('|') + 1;

		const double wY = ((lines > 1 ? maxSize(Label.Text) : Label.Text.size()) * dy) / 2.0;
		const double wX = (lines * dx) / 2.0;

		const QPointF Org = Label.Point;

		const QVector<QPointF> Move =
		{
			{ wX, wY }, { wX, 0.0 }, { wX, -wY },
			{ 0, wY }, { 0.0, 0.0 }, { 0, -wY },
			{ -wX, wY }, { -wX, 0 }, { -wX, -wY }
		};

		Label.Point += Move.value(Label.Just - 1); Label.Just = 5;

		Label.Pol.append({ Label.Point.x() - wX, Label.Point.y() + wY });
		Label.Pol.append({ Label.Point.x() + wX, Label.Point.y() + wY });
		Label.Pol.append({ Label.Point.x() + wX, Label.Point.y() - wY });
		Label.Pol.append({ Label.Point.x() - wX, Label.Point.y() - wY });
		Label.Pol.append({ Label.Point.x() - wX, Label.Point.y() + wY });

		for (auto& P : Label.Pol)
		{
			QLineF Line(Org, P);
			Line.setAngle(Line.angle() - Label.Ang);
			P = Line.p2();
		}
	};

	QtConcurrent::blockingMap(Labels, [&Layers, surfLabel] (QList<LABEL>& List) -> void
	{
		for (auto& Label : List) if (Layers.contains(Label.Idw))
		{
			surfLabel(Label, Layers[Label.Idw]);
		}
	});
}

QSet<QPair<double, double>> RedactionWidget::getColisions(const QList<LABEL>& Labels) const
{
	QMutex Locker; QSet<QPair<double, double>> Res;

	QtConcurrent::blockingMap(Labels, [&Labels, &Res, &Locker] (const LABEL& Lab) -> void
	{
		for (const auto& Oth : Labels) if (Lab.Uid != Oth.Uid)
		{
			const QPolygonF Int = Lab.Pol.intersected(Oth.Pol);

			if (!Int.isEmpty())
			{
				int S = Int.size();
				double x(0.0), y(0.0);

				if (Int.isClosed()) S -= 1;

				for (int i = 0; i < S; ++i)
				{
					x += Int[i].x() / S;
					y += Int[i].y() / S;
				}

				Locker.lock();
				Res.insert({ x, y });
				Locker.unlock();
			}
		}
	});

	return Res;
}

QSet<QPair<double, double>> RedactionWidget::getColisions(const QList<LABEL>& Labels, const QList<QPointF>& Symbols) const
{
	QMutex Locker; QSet<QPair<double, double>> Res; const double Scl = smbScales.value(mapScale) / 2.0;

	static const auto pdistance = [] (const QLineF& L, const QPointF& P) -> double
	{
		const double a = QLineF(P.x(), P.y(), L.x1(), L.y1()).length();
		const double b = QLineF(P.x(), P.y(), L.x2(), L.y2()).length();
		const double l = L.length();

		if ((a * a <= l * l + b * b) &&
		    (b * b <= a * a + l * l))
		{
			const double A = P.x() - L.x1(); const double B = P.y() - L.y1();
			const double C = L.x2() - L.x1(); const double D = L.y2() - L.y1();

			return qAbs(A * D - C * B) / qSqrt(C * C + D * D);
		}
		else return INFINITY;
	};

	QtConcurrent::blockingMap(Labels, [&Symbols, &Res, &Locker, Scl] (const LABEL& Lab) -> void
	{
		for (const auto& Smb : Symbols)
		{
			bool OK = Lab.Pol.containsPoint(Smb, Qt::OddEvenFill);

			if (!OK) for (int i = 1; i < Lab.Pol.size(); ++i)
			{
				OK = OK || (pdistance(QLineF(Lab.Pol[i - 1], Lab.Pol[i]), Smb) <= Scl);
			}

			if (OK)
			{
				Locker.lock();
				Res.insert({ Smb.x(), Smb.y() });
				Locker.unlock();
			}
		}
	});

	return Res;
}

void RedactionWidget::refreshButtonClicked(void)
{
	setEnabled(false); QtConcurrent::run(this, &RedactionWidget::refreshData);
}

void RedactionWidget::optionsButtonClicked(void)
{
	RedactionDialog* Dialog = new RedactionDialog(this); Dialog->open();

	connect(Dialog, &RedactionDialog::accepted, Dialog, &RedactionDialog::deleteLater);
	connect(Dialog, &RedactionDialog::rejected, Dialog, &RedactionDialog::deleteLater);

	connect(Dialog, &RedactionDialog::onDialogAccepted, this, &RedactionWidget::setParameters);

	Dialog->setParameters(smbScales, smbExclude, smbCompute);
}

void RedactionWidget::saveButtonClicked(void)
{
	QStandardItemModel* Model = dynamic_cast<QStandardItemModel*>(ui->treeView->model());

	if (!Model) QMessageBox::critical(this, tr("Error"), tr("No valid data to save"));
	else
	{
		const QString Path = QFileDialog::getSaveFileName(this, tr("Save coordinates"),
												QString(), tr("Text files (*.txt)"));

		if (!Path.isEmpty())
		{
			QFile File(Path); QTextStream Stream(&File);

			Stream.setRealNumberNotation(QTextStream::FixedNotation);
			Stream.setRealNumberPrecision(3);

			if (File.open(QFile::WriteOnly | QFile::Text))
			{
				for (int i = 0; i < Model->rowCount(); ++i)
				{
					auto Root = Model->index(i, 0);
					auto Count = Model->rowCount(Root);

					for (int j = 0; j < Count; ++j)
					{
						Stream << Model->data(Model->index(j, 1, Root)).toString() << '\t'
							  << Model->data(Model->index(j, 2, Root)).toString() << '\n';
					}
				}
			}
		}
	}
}

void RedactionWidget::scaleValueChanged(int Scale)
{
	mapScale = SCALE(Scale);
}

void RedactionWidget::dataReloaded(const QHash<QString, QSet<QPair<double, double>>>& Data)
{
	QStandardItemModel* Model = new QStandardItemModel(this);
	const QStringList Bases = Data.keys();

	Model->setHorizontalHeaderLabels(
	{
		tr("Source"), tr("X"), tr("Y")
	});

	for (const auto& Db : Bases)
	{
		QStandardItem* Root = new QStandardItem(Db);

		for (const auto& P : Data[Db])
		{
			QList<QStandardItem*> Row;

			Row << new QStandardItem();
			Row << new QStandardItem(QString::number(P.first, 'f', 3));
			Row << new QStandardItem(QString::number(P.second, 'f', 3));

			Root->appendRow(Row);
		}

		Model->appendRow(Root);
	}

	auto oldModel = ui->treeView->model();
	auto oldSelect = ui->treeView->selectionModel();

	ui->treeView->setModel(Model);

	delete oldModel;
	delete oldSelect;

	setEnabled(true);
}

void RedactionWidget::refreshData(void)
{
	QMutexLocker Locker(&Synchronizer);
	QHash<QString, QSet<QPair<double, double>>> List;
	QList<LABEL> allLabels; QList<QPointF> allSmbs;

	auto Databases = Core->getDatabases();

	for (int i = 0; i < Databases.size(); ++i)
	{
		const auto Tables = loadTables(Databases[i]);
		const auto Layers = loadLayers(Databases[i]);

		auto Labels = loadLabels(Databases[i]);

		for (const auto& Tab : Tables)
		{
			parseLabels(Labels, Tab, loadData(Tab, Databases[i]));
		}

		if (Labels.size()) surfLabels(Labels, mapScale, Layers);

		const QString dbName = Databases[i].databaseName();
		QtConcurrent::blockingMap(Labels, [dbName] (QList<LABEL>& L) -> void
		{
			for (auto& l : L) l.Uid = qHash(qMakePair(dbName, l.Uid));
		});

		for (const auto& L : Labels) allLabels.append(L);

		if (smbCompute) allSmbs.append(loadSymbols(Databases[i]));
	}

	List.insert(tr("Labels colisions"), getColisions(allLabels));
	List.insert(tr("Symbols colisions"), getColisions(allLabels, allSmbs));

	emit onDataReloaded(List);
}

bool operator == (const RedactionWidget::FIELD& One, const RedactionWidget::FIELD& Two)
{
	return
	(
		One.Type == Two.Type &&
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Dict == Two.Dict
	);
}

bool operator == (const RedactionWidget::TABLE& One, const RedactionWidget::TABLE& Two)
{
	return
	(
		One.Name == Two.Name &&
		One.Label == Two.Label &&
		One.Data == Two.Data &&
		One.Fields == Two.Fields
	);
}

QVariant getDataFromDict(const QVariant& Value, const QMap<QVariant, QPair<QString, QString>>& Dict, RedactionWidget::TYPE Type, bool Sh)
{
	if (!Value.isValid()) return QVariant();

	if (Type == RedactionWidget::BOOL && Dict.isEmpty())
	{
		return Value.toBool() ? RedactionWidget::tr("Yes") : RedactionWidget::tr("No");
	}

	if (Type == RedactionWidget::MASK && !Dict.isEmpty())
	{
		QStringList Values; const int Bits = Value.toInt();

		for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
		{
			if (Bits & (1 << i.key().toInt())) Values.append(Sh ? i.value().second : i.value().first);
		}

		return Values.join(", ");
	}

	if (Dict.isEmpty()) return Value;

	if (Dict.contains(Value)) return Sh ? Dict[Value].second : Dict[Value].first;
	else return QVariant();
}

QVariant getDataByDict(const QVariant& Value, const QMap<QVariant, QPair<QString, QString>>& Dict, RedactionWidget::TYPE Type, bool Sh)
{
	if (!Value.isValid() || Value.toString() == "NULL") return QVariant();

	if (Type == RedactionWidget::BOOL && Dict.isEmpty())
	{
		if (Value.toString() == RedactionWidget::tr("Yes")) return true;
		else if (Value.toString() == RedactionWidget::tr("No")) return false;
		else return Value.toBool();
	}

	if (Type == RedactionWidget::MASK && !Dict.isEmpty())
	{
		QString List = Value.toString(); int Mask = 0;

		for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
		{
			if (List.contains(Sh ? i.value().second : i.value().first)) Mask |= (1 << i.key().toInt());
		}

		return Mask;
	}

	if (Dict.isEmpty() || Dict.keys().contains(Value)) return Value;

	for (auto i = Dict.constBegin(); i != Dict.constEnd(); ++i)
	{
		if ((Sh ? i.value().second : i.value().first) == Value) return i.key();
	}

	return 0;
}

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
}

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return Item;
}

template<class Type, class Field, template<class> class Container>
bool hasItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer)
{
	for (auto& Item : Items) if (Item.*Pointer == Data) return true; return false;
}
