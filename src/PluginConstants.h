#pragma once

#define PLUGIN_NAME        "REGPOB"
#define PLUGIN_VERSION     "1.3.0"
#define PLUGIN_AUTHOR      "VATSIM ESPlugin"
#define PLUGIN_COPYRIGHT   "MIT License"

// Tag item codes (display).
constexpr int TAG_ITEM_REG = 9001;
constexpr int TAG_ITEM_POB = 9002;

// Left-click opens inline editor (same white box as SPAD / OpenPopupEdit).
constexpr int TAG_FUNC_EDIT_REG = 9001;
constexpr int TAG_FUNC_EDIT_POB = 9002;

// OpenPopupEdit commit callbacks.
constexpr int TAG_FUNC_COMMIT_REG = 9101;
constexpr int TAG_FUNC_COMMIT_POB = 9102;

// Flight strip annotation slots used for controller-local persistence.
constexpr int STRIP_ANNOTATION_REG = 0;
constexpr int STRIP_ANNOTATION_POB = 1;
// Marks which field is being edited via scratchpad bridge (REG or POB).
constexpr int STRIP_ANNOTATION_EDIT_TARGET = 2;
