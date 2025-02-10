#pragma once

#include "Humble2Core.h"

class NewSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
	}

	virtual void OnUpdate(float ts) override
	{
		HBL2_INFO("Hello!");
		NewHelperScript::Print();
	}
};

REGISTER_HBL2_SYSTEM(NewSystem)
