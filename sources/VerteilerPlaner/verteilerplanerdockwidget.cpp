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
#include "verteilerplanerdockwidget.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

/**
	@brief VerteilerPlanerDockWidget::VerteilerPlanerDockWidget
	Constructor. Builds the (skeleton) panel content.
	@param parent : parent widget
*/
VerteilerPlanerDockWidget::VerteilerPlanerDockWidget(QWidget *parent) :
	QDockWidget(tr("Planificateur de tableau", "dock title"), parent)
{
		// Object name so it can be retrieved by stylesheets / saveState().
	setObjectName(QStringLiteral("verteiler_planer_panel"));
	setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	setFeatures(QDockWidget::DockWidgetClosable
				| QDockWidget::DockWidgetMovable
				| QDockWidget::DockWidgetFloatable);

	auto *content = new QWidget(this);
	auto *layout  = new QVBoxLayout(content);

	auto *info = new QLabel(
				tr("Générateur de tableau / schéma basé sur un modèle.\n"
				   "Étape 1 : squelette – modèle de données et génération à venir."),
				content);
	info->setWordWrap(true);

	m_generate_button = new QPushButton(tr("Générer le tableau"), content);
	m_generate_button->setEnabled(false);
	m_generate_button->setToolTip(tr("Pas encore implémenté (étape 2)."));
	connect(m_generate_button, &QPushButton::clicked,
			this, &VerteilerPlanerDockWidget::generateRequested);

	layout->addWidget(info);
	layout->addWidget(m_generate_button);
	layout->addStretch(1);

	setWidget(content);
}
