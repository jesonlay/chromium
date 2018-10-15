// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/x11_display_fetcher_ozone.h"

#include <dlfcn.h>

#include "base/logging.h"
#include "base/memory/protected_memory_cfi.h"
#include "ui/base/x/x11_display_util.h"
#include "ui/base/x/x11_util.h"
#include "ui/display/display.h"
#include "ui/display/util/display_util.h"
#include "ui/display/util/x11/edid_parser_x11.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/x11_types.h"

namespace ui {

namespace {

float GetDeviceScaleFactor() {
  float device_scale_factor = 1.0f;
  // TODO(jkim) : Get device scale factor using scale factor and resolution like
  // 'GtkUi::GetRawDeviceScaleFactor'.
  if (display::Display::HasForceDeviceScaleFactor())
    device_scale_factor = display::Display::GetForcedDeviceScaleFactor();
  return device_scale_factor;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// X11DisplayFetcherOzone, public:

X11DisplayFetcherOzone::X11DisplayFetcherOzone(
    X11DisplayFetcherOzone::Delegate* delegate)
    : xdisplay_(gfx::GetXDisplay()),
      x_root_window_(DefaultRootWindow(xdisplay_)),
      xrandr_version_(ui::GetXrandrVersion(xdisplay_)),
      xrandr_event_base_(0),
      delegate_(delegate) {
  DCHECK(delegate);
  // We only support 1.3+. There were library changes before this and we should
  // use the new interface instead of the 1.2 one.
  int randr_version_major = 0;
  int randr_version_minor = 0;
  if (XRRQueryVersion(xdisplay_, &randr_version_major, &randr_version_minor)) {
    xrandr_version_ = randr_version_major * 100 + randr_version_minor;
  }
  // Need at least xrandr version 1.3.
  std::vector<display::Display> displays;
  float scale = ui::GetDeviceScaleFactor();
  if (xrandr_version_ < 103) {
    displays = GetFallbackDisplayList(scale);
  } else {
    int error_base_ignored = 0;
    XRRQueryExtension(xdisplay_, &xrandr_event_base_, &error_base_ignored);

    if (ui::PlatformEventSource::GetInstance())
      ui::PlatformEventSource::GetInstance()->AddPlatformEventDispatcher(this);
    XRRSelectInput(xdisplay_, x_root_window_,
                   RRScreenChangeNotifyMask | RROutputChangeNotifyMask |
                       RRCrtcChangeNotifyMask);

    const std::vector<display::Display> displays = ui::BuildDisplaysFromXRandRInfo(
        xrandr_version_, scale, &primary_display_index_);
  }
  for (auto& display : displays)
    delegate_->AddDisplay(display, display.id() == primary_display_index_);
}

X11DisplayFetcherOzone::~X11DisplayFetcherOzone() {
  if (xrandr_version_ >= 103 && ui::PlatformEventSource::GetInstance())
    ui::PlatformEventSource::GetInstance()->RemovePlatformEventDispatcher(this);
}

bool X11DisplayFetcherOzone::CanDispatchEvent(const ui::PlatformEvent& event) {
  // TODO(msisov, jkim): implement this.
  NOTIMPLEMENTED_LOG_ONCE();
  return false;
}

uint32_t X11DisplayFetcherOzone::DispatchEvent(const ui::PlatformEvent& event) {
  // TODO(msisov, jkim): implement this.
  NOTIMPLEMENTED_LOG_ONCE();
  return ui::POST_DISPATCH_NONE;
}

}  // namespace ui
