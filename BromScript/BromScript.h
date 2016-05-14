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


// see SIF.h for config
#include "SIF.h"

#include "Objects/Function.h"
#include "Objects/ExecuteData.h"
#include "Objects/ByteReader.h"
#include "Objects/ByteWriter.h"
#include "Objects/CallStack.h"
#include "Objects/Variable.h"
#include "Objects/ArgumentData.h"
#include "Objects/Table.h"
#include "Objects/List.h"
#include "Objects/Userdata.h"
#include "Objects/CompilerException.h"
#include "Objects/RuntimeException.h"
#include "Objects/Environment.h"
#include "Managers/Compiler.h"
#include "Managers/Debugger.h"
#include "Managers/Instance.h"
#include "Managers/Executer.h"
#include "Managers/Converter.h"