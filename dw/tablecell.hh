#ifndef __DW_TABLECELL_HH__
#define __DW_TABLECELL_HH__

#include "core.hh"

namespace dw {

namespace tablecell {

bool getAdjustMinWidth ();
bool isBlockLevel ();

int correctAvailWidthOfChild (core::Widget *widget, core::Widget *child,
                              int width, bool forceValue);
int correctAvailHeightOfChild (core::Widget *widget, core::Widget *child,
                               int height, bool forceValue);

int applyPerWidth (core::Widget *widget, int containerWidth,
                   core::style::Length perWidth);
int applyPerHeight (core::Widget *widget, int containerHeight,
                    core::style::Length perHeight);

} // namespace dw

} // namespace dw

#endif // __DW_TABLECELL_HH__
