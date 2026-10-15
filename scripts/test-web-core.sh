#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
source .cache/emsdk/emsdk_env.sh >/dev/null 2>&1
for test in tutorial_exit_tests tutorial_tests highlight_tests objective_tests tactic_application_tests core_tests unit_tests animation_tests battle_actions_tests battle_events_tests recovery_tests ai_tests movement_tests range_tests physical_resolution_tests experience_tests strike_application_tests attack_controller_tests; do
  node "build-web/$test.js" assets/generated
done
