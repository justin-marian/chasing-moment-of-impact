/**
 * @file
 * Share reference-style parameter sliders across the physics and world interfaces.
 */

#pragma once

#include "client/render_gl/interface_style.h"


namespace render {

/**
 * Draw a section header with the panel's section font.
 *
 * Includes a small top gap and a separator underneath, matching the vertical
 * rhythm every panel uses.
 *
 * @param name  Visible section label and widget identity.
 * @param fonts Renderer-owned typography roles used by the panel.
 */
void section(const char* name, const render::Fonts& fonts);


/**
 * Draw a rectangular button with optional accent-tinted text and border.
 *
 * @param name    Button label.
 * @param width   Requested width, capped to the available space.
 * @param primary When true, draws the label and border in the primary accent color.
 * @return True on the frame the button is clicked.
 */
bool button(const char* name, float width, bool primary = false);


/**
 * Draw a labeled thin-track slider with a circular handle and separate numeric readout.
 *
 * Provides custom track and handle decoration while leaving underlying interaction,
 * keyboard navigation, clamping, and numeric editing native to the framework.
 *
 * @param name   Visible label and scoped widget identity.
 * @param value  Current scalar, updated in place by the native control.
 * @param low    Inclusive lower limit.
 * @param high   Inclusive upper limit, greater than low.
 * @param format Printf-style scalar format, also used by native numeric editing.
 * @param unit   Optional readout suffix; an empty string omits it.
 * @param fonts  Renderer-owned label and numeric fonts, borrowed for this frame.
 * @return True when dragging, navigation or numeric input changes the value.
 */
bool slider(
    const char* name, float* value, 
    float low, float high, 
    const char* format, const char* unit,
    const Fonts& fonts
);

} // namespace render
