#pragma once

#define NUI_MAKE_ICON(x, y) ((((y) & 0xFF) << 8) + ((x) & 0xFF))

//---------------------------------------------------------------------------------------------------------------------

#define NUI_ICON_OK NUI_MAKE_ICON(18, 27)
#define NUI_ICON_CANCEL NUI_MAKE_ICON(7, 28)
#define NUI_ICON_WINDOW_CLOSE NUI_MAKE_ICON(7, 28)
#define NUI_ICON_FILE_NEW NUI_MAKE_ICON(18, 3)
#define NUI_ICON_FILE_OPEN NUI_MAKE_ICON(4, 25)
#define NUI_ICON_FILE_SAVE NUI_MAKE_ICON(25, 2)
#define NUI_ICON_QUIT NUI_MAKE_ICON(25, 28)
#define NUI_ICON_HELP NUI_MAKE_ICON(1, 28)

// Directional/arrow icons
#define NUI_ICON_UP NUI_MAKE_ICON(7, 28)
#define NUI_ICON_DOWN NUI_MAKE_ICON(5, 28)
#define NUI_ICON_LEFT NUI_MAKE_ICON(6, 28)
#define NUI_ICON_RIGHT NUI_MAKE_ICON(4, 28)

// Checked state icon (used by CheckBox)
#define NUI_ICON_CHECK NUI_MAKE_ICON(7, 28)
