/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _GPLATES_GUI_ANIMATIONTIMESDIALOG_H_
#define _GPLATES_GUI_ANIMATIONTIMESDIALOG_H_

#include <wx/dialog.h>
#include <wx/checkbox.h>
#include "global/types.h"

namespace GPlatesGui
{
	class AnimationTimesDialog : public wxDialog
	{
		public:
			AnimationTimesDialog(wxWindow* parent,
			 GPlatesGlobal::fpdata_t start_time,
			 GPlatesGlobal::fpdata_t end_time,
			 GPlatesGlobal::fpdata_t time_delta,
			 bool finish_on_end);
	
			GPlatesGlobal::fpdata_t
			GetStartTime() const;

			GPlatesGlobal::fpdata_t
			GetEndTime() const;

			GPlatesGlobal::fpdata_t
			GetTimeDelta() const;

			bool
			GetFinishOnEnd() const;

		protected:
			static wxString
			fpToWxString(GPlatesGlobal::fpdata_t f);

		private:
			wxTextCtrl* _start_ctrl;
			wxTextCtrl* _end_ctrl;
			wxTextCtrl* _time_delta_ctrl;
			wxCheckBox* _finish_on_end;

			wxString _start_ctrl_str;
			wxString _end_ctrl_str;
			wxString _time_delta_ctrl_str;
	};
}

#endif  /* _GPLATES_GUI_ANIMATIONTIMESDIALOG_H_ */
