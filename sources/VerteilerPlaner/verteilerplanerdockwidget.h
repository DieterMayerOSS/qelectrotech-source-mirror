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

class QPushButton;

/**
	@brief The VerteilerPlanerDockWidget class
	Entry point (dock panel) for the model-based distribution board /
	Stromlaufplan generator ("Weg A"). Milestone 1 provides the skeleton only:
	the panel and a (disabled) generate button. The data model, rule engine
	and folio generator are wired in later milestones.
*/
class VerteilerPlanerDockWidget : public QDockWidget
{
		Q_OBJECT

	public:
		explicit VerteilerPlanerDockWidget(QWidget *parent = nullptr);

	signals:
		/// Emitted when the user asks to generate the distribution board.
		void generateRequested();

	private:
		QPushButton *m_generate_button = nullptr;
};

#endif // VERTEILERPLANERDOCKWIDGET_H
