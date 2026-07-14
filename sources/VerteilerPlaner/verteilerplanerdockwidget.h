/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef VERTEILERPLANERDOCKWIDGET_H
#define VERTEILERPLANERDOCKWIDGET_H

#include <QDockWidget>

#include "verteilermodel.h"

class QLineEdit;
class QPushButton;
class QTableWidget;

/**
	@brief The VerteilerPlanerDockWidget class
	Dock panel for the model-based distribution board / Stromlaufplan generator
	("Weg A"). It hosts an editable table of circuits (add/remove rows); the
	generator (VerteilerGenerator) builds a folio from model().
*/
class VerteilerPlanerDockWidget : public QDockWidget
{
		Q_OBJECT

	public:
		explicit VerteilerPlanerDockWidget(QWidget *parent = nullptr);

			/// Current circuits, read from the table (empty rows are skipped).
		VerteilerModel model() const;

			/// Current project-level settings (title / author / drawing number).
		VerteilerConfig config() const;

	signals:
		/// Emitted when the user asks to generate the distribution board.
		void generateRequested();

	private:
		void appendRow(const QString &bmk = QString(),
					   const QString &rating = QString(),
					   const QString &load = QString());

		QLineEdit *m_title_edit = nullptr;
		QLineEdit *m_author_edit = nullptr;
		QLineEdit *m_drawing_edit = nullptr;
		QTableWidget *m_table = nullptr;
		QPushButton *m_generate_button = nullptr;
};

#endif // VERTEILERPLANERDOCKWIDGET_H
