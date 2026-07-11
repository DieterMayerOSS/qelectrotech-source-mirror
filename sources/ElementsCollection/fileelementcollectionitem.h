/*
		Copyright 2006-2026 The QElectroTech Team
		This file is part of QElectroTech.

		QElectroTech is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation, either version 2 of the License, or
		(at your option) any later version.

		QElectroTech is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FILEELEMENTCOLLECTIONITEM2_H
#define FILEELEMENTCOLLECTIONITEM2_H

#include "elementcollectionitem.h"
#include "elementslocation.h"

class FileElementCollectionItem;

/**
	@brief Plain, thread-safe bundle of a file item's parsed data.
	parseSetupData() runs on a worker thread and must not touch the
	QStandardItem; extractSetupData() (reads the item) and applySetupData()
	(mutates the item) run on the GUI thread. This lets the expensive file
	read + XML parsing happen in parallel while the QStandardItem mutations
	stay on the GUI thread (required under Qt6).
*/
struct FileElementSetupData
{
	FileElementCollectionItem *item = nullptr;
		// inputs, captured on the GUI thread
	bool is_dir = false;
	bool is_element = false;
	bool is_collection_root = false;
	QString m_path;
	QString file_system_path;
	QString collection_path;
		// outputs, computed by the worker
	Qt::ItemFlags flags = Qt::NoItemFlags;
	bool set_text = false;
	QString display_name;
	bool set_search = false;
	QString search_data;
	QString tool_tip;
};

/**
	@brief The FileElementCollectionItem class
	This class specialise ElementCollectionItem for manage a collection in
	a file system. They represente a directory or an element.
*/
class FileElementCollectionItem : public ElementCollectionItem
{
	public:
		FileElementCollectionItem();

		enum { Type = UserType+2 };
		int type() const override { return Type;}

		bool setRootPath(const QString& path,
				 bool set_data = true,
				 bool hide_element = false);
		QString fileSystemPath() const;
		QString dirPath() const;

		bool isDir() const override;
		bool isElement() const override;
		QString localName() override;
		QString localName(const ElementsLocation &location);
		QString name() const override;
		QString collectionPath() const override;
		bool isCollectionRoot() const override;
		bool isCommonCollection() const;
		bool isCompanyCollection() const;
		bool isCustomCollection() const;
		bool isMacrosCollection() const;
		void addChildAtPath(const QString &collection_name) override;

		void setUpData() override;
		void setUpIcon() override;

			// Split of setUpData() for parallel loading: extract (GUI thread,
			// reads the item), parse (worker thread, no item access), apply
			// (GUI thread, mutates the item). setUpData() composes the three.
		FileElementSetupData extractSetupData();
		static void parseSetupData(FileElementSetupData &data);
		void applySetupData(const FileElementSetupData &data);

	private:
		void setPathName(const QString& path_name,
				 bool set_data = true,
				 bool hide_element = false);
		void populate(bool set_data = true, bool hide_element = false);

	private:
		QString m_path;
};

#endif // FILEELEMENTCOLLECTIONITEM2_H
