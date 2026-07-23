# SPDX-License-Identifier: Apache-2.0

idf_component_register(
    SRCS
        "app_main.c"
        "ui/ui_app.c"
        "ui/ui_model.c"
    INCLUDE_DIRS
        "."
        "ui"
)
