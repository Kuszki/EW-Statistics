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

#ifndef REDACTIONWIDGET_HPP
#define REDACTIONWIDGET_HPP

#include <QFileDialog>
#include <QMessageBox>
#include <QWidget>

#include "commonstructures.hpp"
#include "applicationcore.hpp"
#include "redactiondialog.hpp"

namespace Ui
{
	class RedactionWidget;
}

class RedactionWidget : public QWidget
{

		Q_OBJECT

	public: enum SCALE
	{
		S500,
		S1000,
		S2000,
		S5000
	};

	public: enum TYPE
	{
		READONLY	= 0,
		STRING	= 1,
		INTEGER	= 4,
		SMALLINT	= 5,
		BOOL		= 7,
		DOUBLE	= 8,
		DATE		= 101,
		MASK		= 102,
		DATETIME	= 1000
	};

	public: struct FIELD
	{
		TYPE Type;

		QString Name;
		QString Label;

		bool Missing;

		QMap<QVariant, QPair<QString, QString>> Dict;
	};

	public: struct TABLE
	{
		QString Name;
		QString Label;
		QString Data;

		bool Point;
		int Type;

		QList<FIELD> Fields;
		QMap<QString, QString> Labels;
	};

	public: struct LABEL
	{
		QString Dbn;
		int Uid;

		QPointF Point;
		QString Text;

		int Idw;
		int Just;

		double Ang;

		QPolygonF Pol;

		double Rad;
		double Sur;

		QPointF Vect;
		QPointF Org;

		bool Mod = false;
	};

	public: struct SYMBOL
	{
		QPointF Point;
		QSet<int> Labels;
	};

	public: struct LINE
	{
		QList<QLineF> Lines;
		QSet<int> Labels;
	};

	private:

		mutable QMutex Synchronizer;

		ApplicationCore* Core;
		Ui::RedactionWidget* ui;

		QVector<double> smbScales;
		QStringList smbExclude;
		SCALE mapScale = S500;

		bool smbCompute;
		double tolAbs;
		double tolPrc;
		int maxIters;

	public:

		explicit RedactionWidget(ApplicationCore* App, QWidget* Parent = nullptr);
		virtual ~RedactionWidget(void) override;

	public slots:

		void setParameters(const QVector<double>& Scales,
					    const QStringList& Exclude,
					    bool computeSymbols,
					    double absValue,
					    double prcValue,
					    int itCount);

	protected:

		QList<TABLE> loadTables(QSqlDatabase& Db);

		QHash<int, QVector<double>> loadLayers(QSqlDatabase& Db);

		QHash<int, QList<QVariant>> loadData(const TABLE& Table,
									  QSqlDatabase& Db);

		QHash<int, QList<LABEL>> loadLabels(QSqlDatabase& Db);

		QList<QPointF> loadSymbols(QSqlDatabase& Db);

		QHash<QString, QList<SYMBOL>> loadSmbLabels(void);
		QHash<QString, QList<LINE>> loadLinLabels(void);

		void parseLabels(QHash<int, QList<LABEL>>& Labels, const TABLE& Tab,
					  const QHash<int, QList<QVariant>>& Values) const;

		void surfLabels(QHash<int, QList<LABEL>>& Labels, SCALE Sc,
					 const QHash<int, QVector<double>>& Layers) const;

		QSet<QStringList> getColisions(QList<LABEL>& Labels) const;
		QSet<QStringList> getColisions(QList<LABEL>& Labels,
								 const QList<QPointF>& Symbols) const;

		int fixColisions(QList<LABEL>& Labels,
					  const QHash<QString, QList<SYMBOL>>& Symbols,
					  const QHash<QString, QList<LINE>>& Lines);

		QSet<QStringList> saveChanges(const QList<LABEL>& Labels);

	private slots:

		void refreshButtonClicked(void);
		void optionsButtonClicked(void);
		void saveButtonClicked(void);
		void fixButtonClicked(void);

		void scaleValueChanged(int Scale);

		void dataReloaded(const QSet<QStringList>& Data);

		void selectionChanged(void);

		void refreshData(void);
		void fixRedaction(void);

	signals:

		void onDataReloaded(const QSet<QStringList>&);

		void onProgressRename(const QString&) const;
		void onProgressStart(int, int) const;
		void onProgresUpdate(int) const;

};

bool operator == (const RedactionWidget::FIELD& One, const RedactionWidget::FIELD& Two);
bool operator == (const RedactionWidget::TABLE& One, const RedactionWidget::TABLE& Two);

QVariant getDataFromDict(const QVariant& Value, const QMap<QVariant, QPair<QString, QString>>& Dict, RedactionWidget::TYPE Type, bool Sh);
QVariant getDataByDict(const QVariant& Value, const QMap<QVariant, QPair<QString, QString>>& Dict, RedactionWidget::TYPE Type, bool Sh);

template<class Type, class Field, template<class> class Container>
Type& getItemByField(Container<Type>& Items, const Field& Data, Field Type::*Pointer);

template<class Type, class Field, template<class> class Container>
const Type& getItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer);

template<class Type, class Field, template<class> class Container>
bool hasItemByField(const Container<Type>& Items, const Field& Data, Field Type::*Pointer);

#endif // REDACTIONWIDGET_HPP
