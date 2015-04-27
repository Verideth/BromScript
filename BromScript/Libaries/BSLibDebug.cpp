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

#include "BSLibDebug.h"
#include "../SIF.h"
#include "../Objects/ArgumentData.h"
#include "../Objects/Variable.h"

using namespace Scratch;

namespace BromScript{
	namespace Libaries{
		namespace Debug{
			BS_FUNCTION(Print) {
				bsi->Debug->Print(args->GetString(0));
				return null;
			}

			BS_FUNCTION(Connect) {
				return Converter::ToVariable(bsi, bsi->Debug->Connect());
			}

			BS_FUNCTION(Disconnect) {
				bsi->Debug->Disconnect();
				return null;
			}

			BS_FUNCTION(Break) {
				bsi->Debug->Break();
				return null;
			}
		}
	}
}