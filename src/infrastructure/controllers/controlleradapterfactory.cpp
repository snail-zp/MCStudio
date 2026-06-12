#include "infrastructure/controllers/controlleradapterfactory.h"

#include <memory>

#include "infrastructure/acs/acscontroller.h"

std::unique_ptr<IControllerAdapter> ControllerAdapterFactory::create(ControllerBrand brand)
{
    switch (brand) {
    case ControllerBrand::Acs:
    default:
        return std::make_unique<AcsController>();
    }
}
