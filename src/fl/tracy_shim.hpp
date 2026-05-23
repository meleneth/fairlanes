#pragma once

#if defined(FAIRLANES_ENABLE_TRACY) && FAIRLANES_ENABLE_TRACY

  #include <tracy/Tracy.hpp>

#else

  #define ZoneScoped
  #define ZoneScopedN(name)
  #define ZoneNamed(var, active)
  #define ZoneNamedN(var, name, active)
  #define ZoneScopedC(color)
  #define ZoneScopedNC(name, color)

  #define FrameMark
  #define FrameMarkNamed(name)

  #define TracyPlot(name, value)
  #define TracyMessage(txt, size)
  #define TracyMessageL(txt)
  #define TracyAlloc(ptr, size)
  #define TracyFree(ptr)

#endif
