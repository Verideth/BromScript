#ifndef BROMSCRIPT_USERDATAINSTANCE_INCLUDED
#define BROMSCRIPT_USERDATAINSTANCE_INCLUDED

#include "../SIF.h"
#include "../Objects/Userdata.h"

namespace BromScript{
	class UserdataInstance {
	public:
		void* Ptr;
		Userdata* TypeData;
		bool CallDTor;


		UserdataInstance(void);
		~UserdataInstance(void);

		BSFunction GetOperator(const Operators& op);
		Variable* GetMember(const Scratch::CString& key, bool raw = false);
		Variable* GetMethod(const Scratch::CString& key);
		Variable* GetIndex(Variable* selfobj, Variable* keyvar);
		void SetIndex(Variable* selfobj, Variable* keyvar, Variable* value);
	};
}

#endif