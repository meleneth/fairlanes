#pragma once

#include "variant_bus.hpp"

#include <optional>
#include <type_traits>
#include <utility>

namespace seerin {

// Identity wire: listen for T on src, emit T into dst (wrapped in dst's
// variant).
template <class T, class SrcVariant, class DstVariant>
auto wire(VariantBus<SrcVariant> &src, VariantBus<DstVariant> &dst) {
  static_assert(std::is_constructible_v<DstVariant, T>,
                "wire<T>: destination variant cannot be constructed from T");

  // Return value is whatever Subscription/token VariantBus::on<T>() returns.
  return src.template subscribe<T>(
      [&dst](const T &t) { dst.emit(DstVariant{t}); });
}

// Mapping wire: listen for T on src, map to optional<U>, emit U into dst.
template <class T, class SrcVariant, class DstVariant, class MapFn>
auto wire_map(VariantBus<SrcVariant> &src, VariantBus<DstVariant> &dst,
              MapFn &&map) {
  // map(t) -> std::optional<U> (or something optional-like)
  return src.template subscribe<T>(
      [&dst, map = std::forward<MapFn>(map)](const T &t) mutable {
        auto out = map(t);
        if (!out) {
          return;
        }
        dst.emit(DstVariant{*out});
      });
}

// Variant-to-variant wire: map whole source variant to optional<dst-variant>
template <class SrcVariant, class DstVariant, class MapFn>
auto wire_variant(VariantBus<SrcVariant> &src, VariantBus<DstVariant> &dst,
                  MapFn &&map) {
  return src.template subscribe<SrcVariant>(
      [&dst, map = std::forward<MapFn>(map)](const SrcVariant &v) mutable {
        auto out = map(v); // optional<DstVariant>
        if (!out) {
          return;
        }
        dst.emit(*out);
      });
}

} // namespace seerin
