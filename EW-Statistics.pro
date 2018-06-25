#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#*                                                                         *
#*  Compute various statistics for EWMAPA software                         *
#*  Copyright (C) 2016  Łukasz "Kuszki" Dróżdż  l.drozdz@openmailbox.org   *
#*                                                                         *
#*  This program is free software: you can redistribute it and/or modify   *
#*  it under the terms of the GNU General Public License as published by   *
#*  the  Free Software Foundation, either  version 3 of the  License, or   *
#*  (at your option) any later version.                                    *
#*                                                                         *
#*  This  program  is  distributed  in the hope  that it will be useful,   *
#*  but WITHOUT ANY  WARRANTY;  without  even  the  implied  warranty of   *
#*  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   *
#*  GNU General Public License for more details.                           *
#*                                                                         *
#*  You should have  received a copy  of the  GNU General Public License   *
#*  along with this program. If not, see http://www.gnu.org/licenses/.     *
#*                                                                         *
#* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

QT			+=	core gui widgets concurrent sql printsupport

TARGET		=	EW-Statistics
TEMPLATE		=	app

CONFIG		+=	c++14

SOURCES		+=	main.cpp \
				commonstructures.cpp \
				aboutdialog.cpp \
				mainwindow.cpp \
				databasesdialog.cpp \
				applicationcore.cpp \
				paymentwidget.cpp \
				recordmodel.cpp \
				paymentdialog.cpp \
				connectdialog.cpp \
				redactionwidget.cpp \
				redactiondialog.cpp

HEADERS		+=	commonstructures.hpp \
				mainwindow.hpp \
				aboutdialog.hpp \
				databasesdialog.hpp \
				applicationcore.hpp \
				paymentwidget.hpp \
				recordmodel.hpp \
				paymentdialog.hpp \
				connectdialog.hpp \
				redactionwidget.hpp \
				redactiondialog.hpp

FORMS		+=	mainwindow.ui \
				aboutdialog.ui \
				databasesdialog.ui \
				paymentwidget.ui \
				paymentdialog.ui \
				connectdialog.ui \
				redactionwidget.ui \
				redactiondialog.ui

RESOURCES		+=	resources.qrc

TRANSLATIONS	+=	ew-statistics_pl.ts
