/*
	BromScript - On Runtime Scripting Language.
	Copyright (C) 2012 - 2015  Alex Brouwer (Bromvlieg: http://brom.4o3.nl/)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#ifndef BROMSCRIPT_GARBAGECOLLECTION_INCLUDED
#define BROMSCRIPT_GARBAGECOLLECTION_INCLUDED

#include "../SIF.h"
#include "../Objects/Variable.h"

namespace BromScript{
	class GarbageCollector {
	public:
		GarbageCollector(void);
		~GarbageCollector(void);

		int Run();
		void RunFrame();

		static BS_FUNCTION(RunWrapper);

		Variable* RegisterVariable();
		Variable* RegisterVariable(Variable* var);
	private:
		int FrameSkip;
		Variable** Buffer;
		int BufferSize;
		int NullStart;

		void AllocateMoreSpace();
	};
}
#endif