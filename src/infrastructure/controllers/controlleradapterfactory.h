#pragma once

#include <memory>

#include "domain/interfaces/icontrolleradapter.h"

class ControllerAdapterFactory
{
public:
    static std::unique_ptr<IControllerAdapter> create(ControllerBrand brand);
};
