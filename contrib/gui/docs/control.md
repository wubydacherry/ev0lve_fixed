# Control
`control` is a base class for any control to be used within GUI.

## ID
Control ID is used to identify the control when finding one or locking the input on one.
It is generally possible to have multiple controls with the same ID, tho it's not 
recommended so to avoid problems with locking. Using same IDs with layouts is
usually fine, but that could have problems with scripts.

## Positioning
Usually all control positions are relative to parent control. It is possible to
get an absolute position using `pos_abs()` method. Size is always absolute.

## Containers
If you want to make a control contain and handle children, you should use
`control_container`. Using `container` is incorrect as you will have to implement
your own handlers (can be useful SOMETIMES)

# Methods

| Method   | Description |
| -------- | ----------- |
| control (id) | Base constructor |
| control (id, pos, size) | Base constructor. Allows to specify wanted position and size |
| make_width_automatic() | Will adjust width depending on parent control |
| make_height_automatic() | Will adjust height depending on parent control |
| make_invisible() | Will set `is_visible` to false |
| adjust_margin(margin) | Sets margin to a new value `margin` |
| make_tooltip(text) | Sets tooltip on a current element |
| pos_abs(): vec2 | Returns absolute position on screen |
| area(include_margin): rect | Returns relative area |
| area_abs(include_margin): rect | Returns absolute area |
| is_out_of_rect(rect): bool | Returns true if current control is out of absolute rect `rect` |
| find_associated_window(): window | Returns associated window or `nullptr` if there's none |
| as<type>(): type | Type convertor |

# Fields

| Field | Description |
| ----- | ----------- |
| id | Control ID |
| parent | Parent control |
| window | Associated window |
| priority | Input processing priority |
| sort | Used to process and render controls as user expects |
| size_to_parent_w | Automatic width |
| size_to_parent_h | Automatic height |
| is_container | True if this control is a container |
| is_window | True if this control is a window |
| is_action_popup | True if this control is a popup that should be auto-closed if clicked outside the area |
| is_visible | True if control is visible |
| is_taking_input | True if control should process input |
| is_taking_keys | True if control should process keyboard input |
| is_taking_text | True if control should process text input |
| pos | Relative position |
| size | Control size |
| margin | Margin to offset the control |
| tooltip | Current tooltip |
| is_mouse_on_me | True if cursor is currently hovers this control |
| is_caring_about_parent | True if control should respect parent control's state |
| is_caring_about_hover | True if control should process hover events |
| hotkey_mode | Hotkey behavior mode for this control (toggle/hold) |
| hotkey_type | Used to determine how value must be set on hotkey press |
| allow_multiple_keys | True if it is possible to bind more than 1 key |
| hotkeys | Hotkey list |