#ifndef __RULER_HH__
#define __RULER_HH__

#include "regardingborder.hh"

namespace dw {

/**
 * \brief Widget for drawing (horizontal) rules.
 *
 * This is really an empty widget, the HTML parser puts a border
 * around it, and drawing is done in dw::core::Widget::drawWidgetBox.
 *
 * Ruler implements RegardingBorder; this way, it is simpler to fit
 * the ruler exactly within the space between floats. Currently, the
 * drawn area of the ruler is too large (but most of the superfluous
 * part is hidden by the floats); this problem will soon solved here
 * in the "dillo_grows" repository.
 */
class Ruler: public RegardingBorder
{
protected:
   void sizeRequestSimpl (core::Requisition *requisition);
   void getExtremesSimpl (core::Extremes *extremes);
   void containerSizeChangedForChildren ();
   bool usesAvailWidth ();
   void draw (core::View *view, core::Rectangle *area,
              core::DrawingContext *context);
   core::Widget *getWidgetAtPoint (int x, int y,
                                   core::GettingWidgetAtPointContext *context);

public:
   static int CLASS_ID;

   Ruler ();
   ~Ruler ();

   bool isBlockLevel ();

   core::Iterator *iterator (core::Content::Type mask, bool atEnd);
};

} // namespace dw

#endif // __RULER_HH__
