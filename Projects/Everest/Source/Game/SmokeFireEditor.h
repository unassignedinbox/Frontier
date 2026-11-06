#pragma once
/*==========================================================================================================================================
                                                          SMOKEFIREDITOR.H
==========================================================================================================================================*/
// 🧩 Native C++ port of the exhibit's Smoke + Fire inspector. Selecting Smoke, Fluid, Plume or Fire in
//    the editor outliner opens the same live gas-domain controls and drives the C++ simulation settings.

#include "Frontier/Engine/Effects/SmokeFluidSystem.h"

namespace Everest
{

class SmokeFireEditor
{
  public:
    void Draw(Frontier::SmokeFluidSettings& Settings, const Frontier::SmokeFluidMetrics& Metrics,
              const std::array<int, 3>& Extent, bool Resident);
    [[nodiscard]] bool ConsumeClearRequest() noexcept;

  private:
    enum class Selection { Smoke, Fluid, Plume, Fire };
    Selection Selected = Selection::Smoke;
    bool ClearRequested = false;
    bool FocusSelection = true;
};

} // namespace Everest
