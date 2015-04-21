#include "../Objects/UserdataInstance.h"


namespace BromScript{
	UserdataInstance::UserdataInstance(void) :CallDTor(false) {
	}

	UserdataInstance::~UserdataInstance(void) {
		if (!this->CallDTor || this->TypeData == null || this->TypeData->Dtor == null || this->Ptr == null)
			return;

		this->TypeData->Dtor(this->TypeData->BromScript, this->Ptr);
		this->Ptr = null;
	}
}