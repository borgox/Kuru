#pragma once

/* The single header a Kuru mod includes. Pulls in the full reflected SDK plus every mod-facing API,
 * so one `#include <Kuru/Kuru.hpp>` gives you:
 *   - IMod + KURU_EXPORT_MOD           (Kuru/Mod.hpp)
 *   - the generated game SDK           (Kuru/Api.hpp -> classes/enums/structs)
 *   - Hooks::Pre/Post                  (Kuru/Hooks.hpp)
 *   - Commands::Register               (Kuru/Commands.hpp)
 *   - Game:: hierarchy helpers         (Kuru/Game.hpp)
 *   - Execute/SetTimeout/IsGameThread  (Kuru/Thread.hpp)
 *   - persistent storage paths         (Kuru/Storage.hpp)
 *   - CVars                            (Kuru/Variables.hpp)
 *   - overlay ImGui binding            (Kuru/Gui.hpp)
 *
 * Prefer this over including the pieces individually.
*/

#include <Kuru/Api.hpp>
#include <Kuru/Mod.hpp>

#include <Kuru/Commands.hpp>
#include <Kuru/Events.hpp>
#include <Kuru/Game.hpp>
#include <Kuru/Gui.hpp>
#include <Kuru/Hooks.hpp>
#include <Kuru/Storage.hpp>
#include <Kuru/Thread.hpp>
#include <Kuru/Variables.hpp>
