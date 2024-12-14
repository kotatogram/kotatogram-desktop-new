# This file is part of Telegram Desktop,
# the official desktop application for the Telegram messaging service.
#
# For license and copyright information please follow this link:
# https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL

option(TDESKTOP_API_TEST "Use test API credentials." OFF)
option(KTGDESKTOP_ENABLE_PACKER "Enable building update packer on non-special targets." OFF)
set(TDESKTOP_API_ID "0" CACHE STRING "Provide 'api_id' for the Telegram API access.")
set(TDESKTOP_API_HASH "" CACHE STRING "Provide 'api_hash' for the Telegram API access.")

if (TDESKTOP_API_TEST)
    set(TDESKTOP_API_ID 17349)
    set(TDESKTOP_API_HASH 344583e45741c457fe1862106095a5eb)
endif()

if (DESKTOP_APP_DISABLE_AUTOUPDATE)
    target_compile_definitions(Telegram PRIVATE TDESKTOP_DISABLE_AUTOUPDATE)
endif()

if (DESKTOP_APP_DISABLE_CRASH_REPORTS)
    target_compile_definitions(Telegram PRIVATE TDESKTOP_DISABLE_CRASH_REPORTS)
endif()

if (DESKTOP_APP_USE_PACKAGED)
    target_compile_definitions(Telegram PRIVATE TDESKTOP_USE_PACKAGED)
endif()

# if (DESKTOP_APP_SPECIAL_TARGET)
#     target_compile_definitions(Telegram PRIVATE TDESKTOP_ALLOW_CLOSED_ALPHA)
# endif()
