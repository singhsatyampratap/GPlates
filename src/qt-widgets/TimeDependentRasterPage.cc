/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <QTableWidgetItem>
#include <QItemDelegate>
#include <QFileDialog>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QHeaderView>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QKeyEvent>
#include <QSizePolicy>
#include <QDebug>

#include "TimeDependentRasterPage.h"

#include "FriendlyLineEdit.h"
#include "ImportRasterDialog.h"

#include "file-io/RasterReader.h"

#include "maths/MathsUtils.h"

#include "utils/Parse.h"


namespace
{
	using namespace GPlatesQtWidgets;

	const double MINIMUM_TIME = 0.0;
	const double MAXIMUM_TIME = 5000.0; // The Earth is only 4.5 billion years old!
	const int DECIMAL_PLACES = 4;

	class TimeLineEdit :
			public FriendlyLineEdit
	{
	public:

		TimeLineEdit(
				const QString &contents,
				const QString &message_on_empty_string,
				QTableWidget *table,
				QWidget *parent_ = NULL) :
			FriendlyLineEdit(contents, message_on_empty_string, parent_),
			d_table(table)
		{
			QSizePolicy policy = lineEditSizePolicy();
			policy.setVerticalPolicy(QSizePolicy::Preferred);
			setLineEditSizePolicy(policy);
		}

		void
		set_model_index(
				const QModelIndex &index)
		{
			d_model_index = index;
		}

	protected:

		virtual
		void
		focusInEvent(
				QFocusEvent *event_)
		{
			// For some reason, the row in the table containing this line edit sometimes
			// gets selected when the line edit gets focus, but sometimes it doesn't.
			// Because Qt can't make up its mind, we'll do it explicitly here.
			d_table->setCurrentIndex(d_model_index);
		}

		virtual
		void
		handle_text_edited(
				const QString &text_)
		{
			d_table->setItem(
					d_model_index.row(),
					d_model_index.column(),
					new QTableWidgetItem(text()));
		}

	private:

		QTableWidget *d_table;
		QModelIndex d_model_index;
	};

	class TimeDelegate :
			public QItemDelegate
	{
	public:

		TimeDelegate(
				QValidator *validator,
				QTableWidget *parent_) :
			QItemDelegate(parent_),
			d_validator(validator),
			d_table(parent_)
		{  }

		virtual
		QWidget *
		createEditor(
				QWidget *parent_,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const
		{
			QString existing = d_table->item(index.row(), index.column())->text();

			static const QString EMPTY_STRING_MESSAGE = "not set";
			GPlatesQtWidgets::FriendlyLineEdit *line_edit =
				new TimeLineEdit(existing, EMPTY_STRING_MESSAGE, d_table, parent_);
			line_edit->setValidator(d_validator);
			line_edit->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

			return line_edit;
		}

		virtual
		void
		setEditorData(
				QWidget *editor,
				const QModelIndex &index) const
		{
			TimeLineEdit *line_edit = dynamic_cast<TimeLineEdit *>(editor);
			if (line_edit)
			{
				line_edit->set_model_index(index);
			}
		}

		virtual
		void
		setModelData(
				QWidget *editor,
				QAbstractItemModel *model,
				const QModelIndex &index) const
		{
			TimeLineEdit *line_edit = dynamic_cast<TimeLineEdit *>(editor);
			if (line_edit)
			{
				QString text = line_edit->text();
				d_table->setItem(index.row(), index.column(), new QTableWidgetItem(text));
			}
		}

	private:

		QValidator *d_validator;
		QTableWidget *d_table;
	};

	class TimeValidator :
			public QDoubleValidator
	{
	public:

		TimeValidator(
				QObject *parent_) :
			QDoubleValidator(parent_)
		{
			setRange(MINIMUM_TIME, MAXIMUM_TIME, DECIMAL_PLACES);
		}

		virtual
		State
		validate(
				QString &input,
				int &pos) const
		{
			if (input.isEmpty() || QDoubleValidator::validate(input, pos) == QValidator::Acceptable)
			{
				return QValidator::Acceptable;
			}
			else
			{
				// This means that values deemed "Intermediate" by the base class are deemed
				// to be invalid.
				return QValidator::Invalid;
			}
		}
	};

	class DeleteKeyEventFilter :
			public QObject
	{
	public:

		DeleteKeyEventFilter(
				const boost::function<void ()> &remove_rows_function,
				QObject *parent_) :
			QObject(parent_),
			d_remove_rows_function(remove_rows_function)
		{  }

	protected:

		bool
		eventFilter(
				QObject *obj,
				QEvent *event_)
		{
			if (event_->type() == QEvent::KeyPress)
			{
				QKeyEvent *key_event = static_cast<QKeyEvent *>(event_);
				if (key_event->key() == Qt::Key_Delete)
				{
					d_remove_rows_function();
					return true;
				}
			}

			return QObject::eventFilter(obj, event_);
		}

	private:

		boost::function<void ()> d_remove_rows_function;
	};
}


GPlatesQtWidgets::TimeDependentRasterPage::TimeDependentRasterPage(
		QString &open_file_path,
		TimeDependentRasterSequence &raster_sequence,
		const boost::function<void (unsigned int)> &set_number_of_bands_function,
		QWidget *parent_) :
	QWizardPage(parent_),
	d_open_file_path(open_file_path),
	d_raster_sequence(raster_sequence),
	d_set_number_of_bands_function(set_number_of_bands_function),
	d_validator(new TimeValidator(this)),
	d_is_complete(false),
	d_show_full_paths(false)
{
	setupUi(this);

	setTitle("Raster File Sequence");
	setSubTitle("Build the sequence of raster files that make up the time-dependent raster.");

	files_table->verticalHeader()->hide();
	files_table->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	files_table->horizontalHeader()->setHighlightSections(false);

	files_table->setTextElideMode(Qt::ElideLeft);
	files_table->setWordWrap(false);
	files_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	files_table->setSelectionMode(QAbstractItemView::ContiguousSelection);
	files_table->setCursor(QCursor(Qt::ArrowCursor));

	files_table->setItemDelegateForColumn(0, new TimeDelegate(d_validator, files_table));

	files_table->installEventFilter(
			new DeleteKeyEventFilter(
				boost::bind(
					&TimeDependentRasterPage::remove_selected_from_table,
					boost::ref(*this)),
				this));

	warning_container_widget->hide();

	remove_selected_button->setEnabled(false);

	make_signal_slot_connections();
}


bool
GPlatesQtWidgets::TimeDependentRasterPage::isComplete() const
{
	return d_is_complete;
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_add_directory_button_clicked()
{
	QString dir_path = QFileDialog::getExistingDirectory(
			this,
			tr("Add Directory"),
			d_open_file_path,
			QFileDialog::ShowDirsOnly);
	if (dir_path.isEmpty())
	{
		return;
	}

	d_open_file_path = dir_path;

	QDir dir(dir_path);
	add_files_to_sequence(dir.entryInfoList());
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_add_files_button_clicked()
{
	const QString &filters = GPlatesFileIO::RasterReader::get_file_dialog_filters();
	QStringList files = QFileDialog::getOpenFileNames(
			this,
			tr("Add Files"),
			d_open_file_path,
			filters);
	if (files.isEmpty())
	{
		return;
	}

	QFileInfo first(files.front());
	d_open_file_path = first.path();

	QList<QFileInfo> info_list;
	BOOST_FOREACH(const QString &file, files)
	{
		info_list.push_back(QFileInfo(file));
	}
	add_files_to_sequence(info_list);
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_remove_selected_button_clicked()
{
	remove_selected_from_table();
}


void
GPlatesQtWidgets::TimeDependentRasterPage::remove_selected_from_table()
{
	QList<QTableWidgetSelectionRange> ranges = files_table->selectedRanges();

	if (ranges.count() == 1)
	{
		const QTableWidgetSelectionRange &range = ranges.at(0);
		d_raster_sequence.erase(
				range.topRow(),
				range.bottomRow() + 1);

		populate_table();
		files_table->clearSelection();

		check_if_complete();
	}
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_sort_by_time_button_clicked()
{
	d_raster_sequence.sort_by_time();
	populate_table();
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_sort_by_file_name_button_clicked()
{
	d_raster_sequence.sort_by_file_name();
	populate_table();
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_show_full_paths_button_toggled(
		bool checked)
{
	d_show_full_paths = checked;
	populate_table();
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_table_selection_changed()
{
	remove_selected_button->setEnabled(
			!files_table->selectedRanges().isEmpty());
}


void
GPlatesQtWidgets::TimeDependentRasterPage::handle_table_cell_changed(
		int row,
		int column)
{
	if (column == 0)
	{
		QString text = files_table->item(row, 0)->text();
		if (text.isEmpty())
		{
			d_raster_sequence.set_time(row, boost::none);
		}
		else
		{
			GPlatesUtils::Parse<double> parse;
			d_raster_sequence.set_time(row, parse(text));
		}

		check_if_complete();
	}
}


void
GPlatesQtWidgets::TimeDependentRasterPage::make_signal_slot_connections()
{
	// Top row buttons.
	QObject::connect(
			add_directory_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_add_directory_button_clicked()));
	QObject::connect(
			add_files_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_add_files_button_clicked()));
	QObject::connect(
			remove_selected_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_remove_selected_button_clicked()));

	// Buttons on right.
	QObject::connect(
			sort_by_time_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_sort_by_time_button_clicked()));
	QObject::connect(
			sort_by_file_name_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_sort_by_file_name_button_clicked()));
	QObject::connect(
			show_full_paths_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(handle_show_full_paths_button_toggled(bool)));

	// The table.
	QObject::connect(
			files_table,
			SIGNAL(itemSelectionChanged()),
			this,
			SLOT(handle_table_selection_changed()));
	QObject::connect(
			files_table,
			SIGNAL(cellChanged(int, int)),
			this,
			SLOT(handle_table_cell_changed(int, int)));
}


void
GPlatesQtWidgets::TimeDependentRasterPage::check_if_complete()
{
	bool is_complete = false;
	QString warning;

	const TimeDependentRasterSequence::sequence_type &sequence = d_raster_sequence.get_sequence();
	if (sequence.size())
	{
		is_complete = true;

		// Create a vector of just the times.
		std::vector<double> times;
		times.reserve(sequence.size());

		// Note if ww got to here, the sequence contains at least one element.
		const std::vector<GPlatesPropertyValues::RasterType::Type> &first_band_types = sequence[0].band_types;
		unsigned int first_width = sequence[0].width;
		unsigned int first_height = sequence[0].height;

		BOOST_FOREACH(const TimeDependentRasterSequence::element_type &elem, sequence)
		{
			if (elem.band_types != first_band_types)
			{
				is_complete = false;
				warning = tr("All raster files in the sequence must have the same number and type of bands.");
				break;
			}

			if (elem.width != first_width || elem.height != first_height)
			{
				is_complete = false;
				warning = tr("All raster files in the sequence must have the same width and height.");
				break;
			}

			if (elem.time)
			{
				times.push_back(*elem.time);
			}
			else
			{
				is_complete = false;
				warning = tr("Please ensure that each raster file has an associated time.");
				break;
			}
		}

		if (is_complete)
		{
			// Sort the times, and see if there are any duplicates.
			// Note if we got to here, sequence contains at least one time.
			std::sort(times.begin(), times.end());
			double prev = times.front();
			for (std::vector<double>::const_iterator iter = ++times.begin(); iter != times.end(); ++iter)
			{
				double curr = *iter;
				if (GPlatesMaths::are_almost_exactly_equal(curr, prev))
				{
					is_complete = false;
					warning = tr("Two or more raster files cannot be assigned the same time.");

					break;
				}

				prev = curr;
			}
		}

		if (is_complete)
		{
			d_set_number_of_bands_function(first_band_types.size());
		}
	}
	else
	{
		warning = tr("The sequence must consist of at least one raster file.");
	}

	if (is_complete)
	{
		warning_container_widget->hide();
	}
	else
	{
		warning_container_widget->show();
		warning_label->setText(warning);
	}

	if (is_complete ^ d_is_complete)
	{
		d_is_complete = is_complete;
		emit completeChanged();
	}
}


void
GPlatesQtWidgets::TimeDependentRasterPage::populate_table()
{
	typedef TimeDependentRasterSequence::sequence_type sequence_type;
	const sequence_type &sequence = d_raster_sequence.get_sequence();

	files_table->setRowCount(sequence.size());

	sequence_type::const_iterator iter = sequence.begin();
	for (unsigned int i = 0; i != sequence.size(); ++i)
	{
		// First column is the time.
		boost::optional<double> time = iter->time;
		QTableWidgetItem *time_item = new QTableWidgetItem(
				time ? QString::number(*time) : QString());
		time_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		time_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
		files_table->setItem(i, 0, time_item);
		files_table->openPersistentEditor(time_item);

		// Second column is the file name.
		QString file_name = d_show_full_paths ? iter->absolute_file_path : iter->file_name;
		QTableWidgetItem *file_item = new QTableWidgetItem(file_name);
		file_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		file_item->setToolTip(iter->absolute_file_path);
		files_table->setItem(i, 1, file_item);

		// Third column is the number of bands.
		unsigned int number_of_bands = iter->band_types.size();
		QTableWidgetItem *bands_item = new QTableWidgetItem(QString::number(number_of_bands));
		bands_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		bands_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		files_table->setItem(i, 2, bands_item);

		++iter;
	}
}


void
GPlatesQtWidgets::TimeDependentRasterPage::add_files_to_sequence(
		const QFileInfoList &file_infos)
{
	if (!file_infos.count())
	{
		return;
	}

	TimeDependentRasterSequence new_sequence;

	BOOST_FOREACH(const QFileInfo &file_info, file_infos)
	{
		// Attempt to read the number of bands in the file.
		QString absolute_file_path = file_info.absoluteFilePath();
		GPlatesFileIO::RasterReader::non_null_ptr_type reader =
			GPlatesFileIO::RasterReader::create(absolute_file_path);
		if (!reader->can_read())
		{
			continue;
		}

		// Check that there's at least one band.
		unsigned int number_of_bands = reader->get_number_of_bands();
		if (number_of_bands == 0)
		{
			continue;
		}

		// The bands must be not UNKNOWN or UNINITIALISED.
		std::vector<GPlatesPropertyValues::RasterType::Type> band_types;
		for (unsigned int i = 1; i <= number_of_bands; ++i)
		{
			GPlatesPropertyValues::RasterType::Type band_type = reader->get_type(i);
			if (band_type == GPlatesPropertyValues::RasterType::UNKNOWN ||
				band_type == GPlatesPropertyValues::RasterType::UNINITIALISED)
			{
				continue;
			}
			band_types.push_back(band_type);
		}

		std::pair<unsigned int, unsigned int> raster_size = reader->get_size();
		if (raster_size.first != 0 && raster_size.second != 0)
		{
			new_sequence.push_back(
					deduce_time(file_info),
					absolute_file_path,
					file_info.fileName(),
					band_types,
					raster_size.first,
					raster_size.second);
		}
	}

	new_sequence.sort_by_time();
	d_raster_sequence.add_all(new_sequence);

	populate_table();
	files_table->scrollToBottom();

	check_if_complete();
}


boost::optional<double>
GPlatesQtWidgets::TimeDependentRasterPage::deduce_time(
		const QFileInfo &file_info)
{
	QString base_name = file_info.completeBaseName();
	QStringList tokens = base_name.split("-", QString::SkipEmptyParts);

	if (tokens.count() < 2)
	{
		return boost::none;
	}

	try
	{
		GPlatesUtils::Parse<double> parse;
		double value = parse(tokens.last());
		
		// Is value in an acceptable range?
		if (MINIMUM_TIME <= value && value <= MAXIMUM_TIME)
		{
			return value;
		}
		else
		{
			return boost::none;
		}
	}
	catch (const GPlatesUtils::ParseError &)
	{
		return boost::none;
	}
}
