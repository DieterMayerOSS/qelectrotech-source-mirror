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

#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

/**
	@brief VerteilerPlanerDockWidget::VerteilerPlanerDockWidget
	Constructor. Builds the editable circuit table and the action buttons.
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

		// Project-level settings (mirrors stromlaufplan.de's "Allgemein" tab).
	m_title_edit   = new QLineEdit(content);
	m_author_edit  = new QLineEdit(content);
	m_drawing_edit = new QLineEdit(content);
		// Demo defaults (overwritten by the user).
	m_title_edit->setText(QStringLiteral("EFH Beispiel"));
	m_author_edit->setText(QStringLiteral("QET"));
	m_drawing_edit->setText(QStringLiteral("1001"));
	auto *form = new QFormLayout;
	form->addRow(tr("Désignation du projet"), m_title_edit);
	form->addRow(tr("Auteur"), m_author_edit);
	form->addRow(tr("N° de plan"), m_drawing_edit);
	layout->addLayout(form);

	m_table = new QTableWidget(0, 3, content);
	m_table->setHorizontalHeaderLabels(
				{ tr("Repère", "circuit table column"),
				  tr("Calibre", "circuit table column"),
				  tr("Récepteur", "circuit table column") });
	m_table->horizontalHeader()->setStretchLastSection(true);
	m_table->verticalHeader()->setVisible(false);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

		// Demo defaults. The repère (BMK) is left empty on purpose to show the
		// generator's auto-numbering (-F1, -F2, ...); a manual value overrides it.
	appendRow(QString(), QStringLiteral("B16A"), QStringLiteral("KG Steckdosen"));
	appendRow(QString(), QStringLiteral("B10A"), QStringLiteral("EG Licht"));
	appendRow(QString(), QStringLiteral("B16A"), QStringLiteral("OG Rolladen"));

	auto *add_button    = new QPushButton(tr("Ajouter"), content);
	auto *remove_button = new QPushButton(tr("Supprimer"), content);
	connect(add_button, &QPushButton::clicked, this, [this]() { appendRow(); });
	connect(remove_button, &QPushButton::clicked, this, [this]() {
		const int row = m_table->currentRow();
		if (row >= 0) {
			m_table->removeRow(row);
		}
	});

	auto *row_buttons = new QHBoxLayout;
	row_buttons->addWidget(add_button);
	row_buttons->addWidget(remove_button);
	row_buttons->addStretch(1);

	m_generate_button = new QPushButton(tr("Générer le tableau"), content);
	connect(m_generate_button, &QPushButton::clicked,
			this, &VerteilerPlanerDockWidget::generateRequested);

	layout->addWidget(m_table);
	layout->addLayout(row_buttons);
	layout->addWidget(m_generate_button);

	setWidget(content);
}

/**
	@brief VerteilerPlanerDockWidget::appendRow
	Append a circuit row to the table.
*/
void VerteilerPlanerDockWidget::appendRow(const QString &bmk,
										  const QString &rating,
										  const QString &load)
{
	const int row = m_table->rowCount();
	m_table->insertRow(row);
	m_table->setItem(row, 0, new QTableWidgetItem(bmk));
	m_table->setItem(row, 1, new QTableWidgetItem(rating));
	m_table->setItem(row, 2, new QTableWidgetItem(load));
}

/**
	@brief VerteilerPlanerDockWidget::model
	@return the circuits currently in the table (rows that are entirely empty
	are skipped).
*/
VerteilerModel VerteilerPlanerDockWidget::model() const
{
	VerteilerModel m;
	for (int row = 0; row < m_table->rowCount(); ++row)
	{
		VerteilerCircuit c;
		if (auto *bmk_item = m_table->item(row, 0)) {
			c.bmk = bmk_item->text().trimmed();
		}
		if (auto *rating_item = m_table->item(row, 1)) {
			c.rating = rating_item->text().trimmed();
		}
		if (auto *load_item = m_table->item(row, 2)) {
			c.load = load_item->text().trimmed();
		}
		if (!c.bmk.isEmpty() || !c.rating.isEmpty() || !c.load.isEmpty()) {
			m.append(c);
		}
	}
	return m;
}

/**
	@brief VerteilerPlanerDockWidget::config
	@return the project-level settings entered by the user.
*/
VerteilerConfig VerteilerPlanerDockWidget::config() const
{
	VerteilerConfig c;
	c.title         = m_title_edit->text().trimmed();
	c.author        = m_author_edit->text().trimmed();
	c.drawingNumber = m_drawing_edit->text().trimmed();
	return c;
}
