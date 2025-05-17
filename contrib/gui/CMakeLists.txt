cmake_minimum_required(VERSION 3.16)
project(gui)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)
include_directories(deps/renderer/include)

add_subdirectory(deps/renderer)
add_library(gui STATIC
		# controls
		src/controls/tab.cpp
		src/controls/list.cpp
		src/controls/popup.cpp
		src/controls/label.cpp
		src/controls/group.cpp
		src/controls/layout.cpp
		src/controls/button.cpp
		src/controls/window.cpp
		src/controls/slider.cpp
		src/controls/hotkey.cpp
		src/controls/spacer.cpp
		src/controls/widget.cpp
		src/controls/loading.cpp
		src/controls/checkbox.cpp
		src/controls/child_tab.cpp
		src/controls/combo_box.cpp
		src/controls/text_input.cpp
		src/controls/hsv_slider.cpp
		src/controls/selectable.cpp
		src/controls/tabs_layout.cpp
		src/controls/color_picker.cpp
		src/controls/search_result.cpp
		src/controls/toggle_button.cpp
		src/controls/control_container.cpp
		src/controls/notification_control.cpp

		# popups
		src/popups/dialog_box.cpp
		src/popups/message_box.cpp
		src/popups/search_popup.cpp
		src/popups/hotkey_popup.cpp
		src/popups/combo_box_popup.cpp
		src/popups/color_picker_popup.cpp
		src/popups/notifications_popup.cpp

		# root
		src/gui.cpp
		src/anim.cpp
		src/input.cpp
		src/helpers.cpp
		src/control.cpp
		src/container.cpp
		src/notify_system.cpp
)

add_compile_definitions(STBTT_STATIC STBRP_STATIC)